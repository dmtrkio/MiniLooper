#pragma once

#include "dsp/dsp.h"
#include "dsp/filter/biquad_filter.h"
#include "dsp/effects/effect_base.h"
#include "dsp/parameter/parameter_tree.h"
#include "dsp/parameter/parameter_view.h"

namespace dsp::effects {
    class GuitarAmp final : public EffectBase
    {
    public:
        GuitarAmp()
        {
            driveParam_.referTo(paramTree_["Drive"].asParameterUnsafe());
            toneParam_.referTo(paramTree_["Tone"].asParameterUnsafe());
            levelParam_.referTo(paramTree_["Level"].asParameterUnsafe());
            dryWetParam_.referTo(paramTree_["DryWet"].asParameterUnsafe());
        }

        void prepare(float sampleRate) override
        {
            static constexpr float kSmoothingMs = 1.0f;
            const auto smoothFrames = kSmoothingMs * sampleRate * 0.001f;

            drive.setSmoothingFrames(smoothFrames);
            tone.setSmoothingFrames(smoothFrames);
            level.setSmoothingFrames(smoothFrames);
            dryWet.setSmoothingFrames(smoothFrames);

            drive.init(driveParam_.get());
            tone.init(toneParam_.get());
            level.init(levelParam_.get());
            dryWet.init(dryWetParam_.get());

            preamp.prepare(sampleRate);
            toneStack.prepare(sampleRate);
            cabinet.prepare(sampleRate);
        }

        void process(float *const *data, const unsigned int nFrames) override
        {
            applyParams();

            staticFor<2>([&](auto channel) {
                for (std::size_t frame{}; frame < nFrames; ++frame) {
                    const auto inputSample = data[channel][frame];
                    preamp.setDrive(drive.get<channel>());
                    toneStack.setTone(tone.get<channel>());
                    const auto preampOutput = preamp.processSample<channel>(inputSample);
                    const auto toneStackOutput = toneStack.processSample<channel>(preampOutput);
                    const auto powerAmpOutput = powerAmpDrive(toneStackOutput, drive.get<channel>());
                    const auto cabinetOutput = cabinet.processSample<channel>(powerAmpOutput);
                    const auto processedSample = cabinetOutput * level.get<channel>();
                    data[channel][frame] = std::lerp(inputSample, processedSample, dryWet.get<channel>());
                }
            });
        }

        dsp::parameter::ParameterTree getParameterTree() const override
        {
            return paramTree_;
        }

    private:
        void applyParams() noexcept
        {
            drive = driveParam_.get();
            tone = toneParam_.get();
            level = levelParam_.get();
            dryWet = dryWetParam_.get();
        }

        struct Preamp
        {
            filter::StereoBiquadFilter hp;
            filter::StereoBiquadFilter presenceBoost;
            filter::StereoBiquadFilter lowShelf;
            filter::StereoBiquadFilter lp;

            float driveMin;
            float driveMax;
            float drive;

            Preamp()
                : driveMin(dBtoLinear(6.0f))
                , driveMax(dBtoLinear(30.0f))
                , drive(0.0f)
            {
                setDrive(0.0f);
            }

            void setDrive(float d) noexcept
            {
                drive = std::lerp(driveMin, driveMax, std::clamp(d, 0.0f, 1.0f));
            }

            void prepare(float sampleRate)
            {
                hp.setParameters(filter::FilterType::HighPass, sampleRate, 80.0f);
                lp.setParameters(filter::FilterType::LowPass, sampleRate, 5000.0f);
                presenceBoost.setParameters(
                    filter::FilterType::Peaking,
                    sampleRate,
                    2500.0f,
                    1.0f,
                    std::nullopt,
                    std::nullopt,
                    dBtoLinear(8.0f)
                );
                lowShelf.setParameters(
                    filter::FilterType::LowShelf,
                    sampleRate,
                    150.0f,
                    std::nullopt,
                    std::nullopt,
                    std::nullopt,
                    dBtoLinear(-6.0f)
                );
            }

            template<std::size_t Index>
            float processSample(float sample) noexcept
            {
                float processed = hp.processSample<Index>(sample);
                processed = presenceBoost.processSample<Index>(processed);
                processed = lowShelf.processSample<Index>(processed);
                processed = std::tanh(processed * drive);
                processed = lp.processSample<Index>(processed);
                return processed;
            }
        };

        struct ToneStack
        {
            filter::StereoBiquadFilter low, mid, high, presence;
            float sr;

            void setTone(float tone) noexcept
            {
                float lowGain = 8.0f - tone * 12.0f;
                    low.setParameters(
                        filter::FilterType::LowShelf,
                        sr,
                        100.0f,
                        std::nullopt,
                        std::nullopt,
                        std::nullopt,
                        dBtoLinear(lowGain)
                    );
                    
                    float midGain = (tone - 0.5f) * 6.0f;
                    float midFreq = 400.0f + tone * 600.0f;
                    float midQ = 1.5f - tone * 0.6f;
                    mid.setParameters(
                        filter::FilterType::Peaking,
                        sr,
                        midFreq,
                        midQ,
                        std::nullopt,
                        std::nullopt,
                        dBtoLinear(midGain)
                    );
                    
                    float highGain = -6.0f + tone * 16.0f;
                    float highFreq = 1500.0f + tone * 2000.0f;
                    high.setParameters(
                        filter::FilterType::Peaking,
                        sr,
                        highFreq,
                        0.8f,
                        std::nullopt,
                        std::nullopt,
                        dBtoLinear(highGain)
                    );
                    
                    float presenceGain = (tone > 0.5f)
                        ? ((tone - 0.5f) * 16.0f)
                        : (tone * 4.0f - 2.0f);
                    presence.setParameters(
                        filter::FilterType::HighShelf,
                        sr,
                        4500.0f,
                        std::nullopt,
                        std::nullopt,
                        std::nullopt,
                        dBtoLinear(presenceGain)
                    );
            }

            void prepare(float sampleRate)
            {
                sr = sampleRate;
                setTone(0.5f);
            }

            template<std::size_t Index>
            float processSample(float sample) noexcept
            {
                float processed = low.processSample<Index>(sample);
                processed = mid.processSample<Index>(processed);
                processed = high.processSample<Index>(processed);
                processed = presence.processSample<Index>(processed);
                return processed;
            }
        };

        static float powerAmpDrive(float sample, float drive) noexcept
        {
            return std::tanh(sample * (1.0f + drive * 10.0f));
        }

        struct Cabinet
        {
            filter::StereoBiquadFilter highPass, resonance, notch, lowPass;
            void prepare(float sampleRate)
            {
                highPass.setParameters(filter::FilterType::HighPass, sampleRate, 80.0f);
                resonance.setParameters(
                    filter::FilterType::Peaking,
                    sampleRate,
                    400.0f,
                    2.0f,
                    std::nullopt,
                    std::nullopt,
                    dBtoLinear(4.0f)
                );
                notch.setParameters(filter::FilterType::Notch, sampleRate, 3000.0f, 3.0f);
                lowPass.setParameters(filter::FilterType::LowPass, sampleRate, 5000.0f);
            }

            template <std::size_t Index>
            float processSample(float sample) noexcept
            {
                float processed = highPass.processSample<Index>(sample);
                processed = resonance.processSample<Index>(processed);
                processed = notch.processSample<Index>(processed);
                processed = lowPass.processSample<Index>(processed);
                return processed;
            }
        };

        using Param = dsp::parameter::Parameter;
        using ParamTree = dsp::parameter::ParameterTree;

        ParamTree paramTree_{"GuitarAmp", {
            ParamTree{Param::makeFloat("Drive", 0.0f, dsp::Range{0.0f, 1.0f})},
            ParamTree{Param::makeFloat("Tone", 0.5f, dsp::Range{0.0f, 1.0f})},
            ParamTree{Param::makeFloat("Level", 0.5f, dsp::Range{0.0f, 1.0f})},
            ParamTree{Param::makeFloat("DryWet", 0.0f, dsp::Range{0.0f, 1.0f})},
        }};

        Preamp preamp;
        ToneStack toneStack;
        Cabinet cabinet;

        StereoFloatSmoother drive;
        StereoFloatSmoother tone;
        StereoFloatSmoother level;
        StereoFloatSmoother dryWet;

        parameter::FloatParameterView driveParam_;
        parameter::FloatParameterView toneParam_;
        parameter::FloatParameterView levelParam_;
        parameter::FloatParameterView dryWetParam_;
    };
}