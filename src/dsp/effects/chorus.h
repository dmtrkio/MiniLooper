#pragma once

#include <array>

#include "dsp/dsp.h"
#include "dsp/effects/effect_base.h"
#include "dsp/oscillator.h"
#include "dsp/delay_line.h"
#include "dsp/parameter/parameter_view.h"

namespace dsp::effects {
    class Chorus final : public EffectBase
    {
    public:
        Chorus();

        void prepare(float sampleRate) override;
        void process(float *const *data, const unsigned int nFrames) override;
        dsp::parameter::ParameterTree getParameterTree() const override;
        
    private:
        void applyParams();
        std::pair<float, float> processFrame(std::pair<float, float> input) noexcept;

        struct Voice
        {
            float rateOffset;
            Oscillator lfo;
            std::array<float, 2> minDelays, maxDelays;
            std::array<FractionalDelayLine, 2> delayLines;
            std::pair<float, float> prevInput{};

            void setup(float lfoPhaseOffset, float lfoRateOffset, float minDelayMs, float maxDelayMs, float sampleRate, bool flip) noexcept;
            std::pair<float, float> processFrame(std::pair<float, float> input, float rate, float depth, float feedback, float sampleRate) noexcept;
        };

        float sampleRate_{44100.0f};
        FloatSmoother rate_;
        FloatSmoother depth_;
        FloatSmoother mix_;
        FloatSmoother feedback_;

        std::array<Voice, 4> voices_;

        using Param = parameter::Parameter;
        using ParamTree = parameter::ParameterTree;

        ParamTree paramTree_{"Chorus",
            {
                Param::makeFloat("Rate", 1.0f, {0.2f, 3.0f}),
                Param::makeFloat("Depth", 0.5f, {0.0f, 1.0f}),
                Param::makeFloat("Feedback", 0.0f, {0.0f, 1.0f}),
                Param::makeFloat("Mix", 0.0f, {0.0f, 1.0f}),
            }
        };

        parameter::FloatParameterView rateParam_;
        parameter::FloatParameterView depthParam_;
        parameter::FloatParameterView mixParam_;
        parameter::FloatParameterView feedbackParam_;
    };
}