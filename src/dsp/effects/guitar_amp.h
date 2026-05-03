#pragma once

#include <iostream>

#include "dsp/dsp.h"
#include "dsp/filter/biquad_filter.h"
#include "dsp/effects/effect_base.h"
#include "dsp/parameter/parameter_tree.h"

namespace dsp::effects {
    class GuitarAmp final : public EffectBase
    {
    public:
        void prepare(float sampleRate) override
        {
            static constexpr float kSmoothingMs = 1.0f;
            const auto smoothFrames = kSmoothingMs * sampleRate * 0.001f;

            drive.init(paramTree_["Drive"].asParameterUnsafe().get<float>());
            tone.init(paramTree_["Tone"].asParameterUnsafe().get<float>());
            level.init(paramTree_["Level"].asParameterUnsafe().get<float>());
            dryWet.init(paramTree_["DryWet"].asParameterUnsafe().get<float>());

            drive.setSmoothingFrames(smoothFrames);
            tone.setSmoothingFrames(smoothFrames);
            level.setSmoothingFrames(smoothFrames);
            dryWet.setSmoothingFrames(smoothFrames);

            preamp.prepare(sampleRate);
        }

        void process(float *const *data, const unsigned int nFrames) override
        {
            applyParams();

            staticFor<2>([&](auto channel) {
                for (std::size_t frame{}; frame < nFrames; ++frame) {
                    const auto inputSample = data[channel][frame];
                    preamp.setDrive(drive.get<channel>());
                    const auto preampOutput = preamp.processSample<channel>(inputSample);
                    const auto processedSample = preampOutput * level.get<channel>();
                    data[channel][frame] = std::lerp(inputSample, processedSample, dryWet.get<channel>());
                }
            });
        }

        dsp::parameter::ParameterTree getParameterTree() const noexcept override
        {
            return paramTree_;
        }

    private:
        void applyParams() noexcept
        {
            drive = paramTree_["Drive"].asParameterUnsafe().get<float>();
            tone = paramTree_["Tone"].asParameterUnsafe().get<float>();
            level = paramTree_["Level"].asParameterUnsafe().get<float>();
            dryWet = paramTree_["DryWet"].asParameterUnsafe().get<float>();
        }

        StereoFloatSmoother drive;
        StereoFloatSmoother tone;
        StereoFloatSmoother level;
        StereoFloatSmoother dryWet;

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

        Preamp preamp;

        using Param = dsp::parameter::Parameter;
        using ParamTree = dsp::parameter::ParameterTree;

        ParamTree paramTree_{"GuitarAmp", {
            ParamTree{Param::makeFloat("Drive", 0.0f, dsp::Range{0.0f, 1.0f})},
            ParamTree{Param::makeFloat("Tone", 0.5f, dsp::Range{0.0f, 1.0f})},
            ParamTree{Param::makeFloat("Level", 0.5f, dsp::Range{0.0f, 1.0f})},
            ParamTree{Param::makeFloat("DryWet", 0.5f, dsp::Range{0.0f, 1.0f})},
        }};
    };
}