#pragma once

#include "dsp/dsp.h"
#include "dsp/parameter/parameter_tree.h"
#include "dsp/effects/guitar_amp.h"
#include "dsp/effects/chorus.h"
#include "dsp/effects/equalizer.h"
#include "dsp/effects/pitch_shifter.h"

namespace ml::looper {
    class ProcessingChain final : public dsp::effects::EffectBase
    {
    public:
        ProcessingChain() : paramTree_(buildParameterTree()) {}

        void prepare(float sampleRate) override
        {
            static constexpr float kSmoothingMs = 1.0f;
            const auto smoothFrames = kSmoothingMs * sampleRate * 0.001f;

            linearGain.init(dsp::dBtoLinear(paramTree_["GainDb"].asParameterUnsafe().get<float>()));
            pan.init(paramTree_["Pan"].asParameterUnsafe().get<float>());
            linearGain.setSmoothingFrames(smoothFrames);
            pan.setSmoothingFrames(smoothFrames);

            pitchShifter.prepare(sampleRate);
            guitarAmp.prepare(sampleRate);
            chorus.prepare(sampleRate);
            eq.prepare(sampleRate);
        }

        void process(float *const *data, const unsigned int nFrames) override
        {
            applyParams();

            pitchShifter.process(data, nFrames);
            guitarAmp.process(data, nFrames);
            chorus.process(data, nFrames);
            eq.process(data, nFrames);

            auto [leftGain, rightGain] = dsp::equalPowerPanGains(pan());
            const auto gainScalar = linearGain();
            leftGain *= gainScalar;
            rightGain *= gainScalar;

            for (auto i{0u}; i < nFrames; ++i) {
                const auto leftSample = data[0][i] * leftGain;;
                const auto rightSample = data[1][i] * rightGain;
                data[0][i] = leftSample;
                data[1][i] = rightSample;
            }
        }

        dsp::parameter::ParameterTree getParameterTree() const noexcept override { return paramTree_; }

    private:
        using Param = dsp::parameter::Parameter;
        using ParamTree = dsp::parameter::ParameterTree;

        void applyParams()
        {
            linearGain.setTarget(dsp::dBtoLinear(paramTree_["GainDb"].asParameterUnsafe().get<float>()));
            pan.setTarget(paramTree_["Pan"].asParameterUnsafe().get<float>());
        }

        ParamTree buildParameterTree() const
        {
            return {"ProcessingChain", {
                ParamTree{Param::makeFloat("GainDb", 0.0f, dsp::Range{-60.0f, 12.0f})},
                ParamTree{Param::makeFloat("Pan", 0.0f, dsp::Range{-1.0f, 1.0f})},
                pitchShifter.getParameterTree(),
                guitarAmp.getParameterTree(),
                chorus.getParameterTree(),
                eq.getParameterTree(),
            }};
        }

        dsp::FloatSmoother linearGain;
        dsp::FloatSmoother pan;

        dsp::effects::PitchShifter pitchShifter;
        dsp::effects::GuitarAmp guitarAmp;
        dsp::effects::Chorus chorus;
        dsp::effects::Equalizer eq;

        ParamTree paramTree_;
    };
}