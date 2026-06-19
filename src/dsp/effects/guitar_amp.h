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
        GuitarAmp();

        void prepare(float sampleRate) override;
        void process(float *const *data, const unsigned int nFrames) override;
        dsp::parameter::ParameterTree getParameterTree() const override;

    private:
        void applyParams() noexcept;

        struct Preamp
        {
            filter::StereoBiquadFilter hp;
            filter::StereoBiquadFilter presenceBoost;
            filter::StereoBiquadFilter lowShelf;
            filter::StereoBiquadFilter lp;

            float driveMin;
            float driveMax;
            float drive;

            Preamp();

            void prepare(float sampleRate);
            void setDrive(float d) noexcept;

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

            void prepare(float sampleRate);
            void setTone(float tone) noexcept;

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

        struct Cabinet
        {
            filter::StereoBiquadFilter highPass, resonance, notch, lowPass;

            void prepare(float sampleRate);

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