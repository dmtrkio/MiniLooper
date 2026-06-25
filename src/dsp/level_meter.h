#pragma once

#include "dsp/dsp.h"
#include "dsp/filter/ballistics_filter.h"

namespace ml::dsp {
    class LevelMeter
    {
    public:
        void prepare(const float sampleRate)
        {
            constexpr float rmsMs = 5.0f;
            const auto rmsSize = rmsMs * sampleRate * 0.001f;

            rmsL.prepare(rmsSize);
            rmsR.prepare(rmsSize);

            constexpr float attackMs = 10.0f;
            constexpr float releaseMs = 300.0f;
            constexpr float initial = -100.0f;

            ballisticsL.prepare(sampleRate, attackMs, releaseMs, initial);
            ballisticsR.prepare(sampleRate, attackMs, releaseMs, initial);
        }

        void operator()(const float *left, const float *right, const unsigned int nFrames)
        {
            for (auto i{0u}; i < nFrames; ++i) {
                this->operator()(left[i], right[i]);
            }
        }

        void operator()(const float left, const float right)
        {
            ballisticsL.process(linearToDb(rmsL(left)));
            ballisticsR.process(linearToDb(rmsR(right)));
        }

        [[nodiscard]] std::pair<float, float> getLevel() const noexcept
        {
            return {ballisticsL.getState(), ballisticsR.getState()};
        }

    private:
        Rms rmsL{};
        Rms rmsR{};
        filter::BallisticsFilter ballisticsL{};
        filter::BallisticsFilter ballisticsR{};
    };
}