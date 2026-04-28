#pragma once

#include "dsp/dsp.h"
#include "dsp/level_meter.h"
#include "dsp/effects/equalizer.h"

namespace looper {
    struct ProcessingChain
    {
        dsp::FloatSmoother linearGain;
        dsp::FloatSmoother pan;
        dsp::LevelMeter meter;
        dsp::effects::Equalizer eq;

        void prepare(unsigned int sampleRate)
        {
            static constexpr float kSmoothingMs = 1.0f;
            const auto smoothFrames = kSmoothingMs * sampleRate * 0.001f;

            linearGain.init(1.0f);
            pan.init(0.0f);
            linearGain.setSmoothingFrames(smoothFrames);
            pan.setSmoothingFrames(smoothFrames);

            meter.prepare(sampleRate);

            eq.prepare(sampleRate);
        }

        void process(float *const *data, const unsigned int nFrames)
        {
            eq.process(data, nFrames);

            auto [leftGain, rightGain] = dsp::equalPowerPanGains(pan());
            const auto gainScalar = linearGain();
            leftGain *= gainScalar;
            rightGain *= gainScalar;

            for (auto i{0u}; i < nFrames; ++i) {
                const auto leftSample = data[0][i] * leftGain;;
                const auto rightSample = data[1][i] * rightGain;
                meter(leftSample, rightSample);
                data[0][i] = leftSample;
                data[1][i] = rightSample;
            }
        }
    };
}