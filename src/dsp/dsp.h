#pragma once

#include <cmath>
#include <vector>

namespace dsp {
    constexpr float linearToDb(const float value, const float minDb = -100.0f)
    {
        if (value <= 0.0000001f)
            return minDb;

        return 20.0f * std::log10(value);
    }

    inline float dBtoLinear(const float value) noexcept
    {
        static constexpr float scale = 1.f / 20.f;
        return std::pow<float>(10.0f, value * scale);
    }

    class Rms
    {
    public:
        explicit Rms(const std::size_t windowSize = 64)
        {
            reset(windowSize);
        }

        void reset(const std::size_t windowSize)
        {
            buffer_.resize(windowSize, 0.0f);
            sum_ = 0.0f;
            index_ = 0;
        }

        void updateSum(const float sample)
        {
            const float squared = sample * sample;
            sum_ -= buffer_[index_];
            sum_ += squared;
            buffer_[index_] = squared;
            index_ = (index_ + 1) % buffer_.size();
        }

        [[nodiscard]] float getRms() const noexcept
        {
            return sum_ / static_cast<float>(buffer_.size());
        }

        float processSample(const float sample)
        {
            updateSum(sample);
            return getRms();
        }

        float processBlock(const float* samples, const std::size_t nSamples)
        {
            for (auto i{0u}; i < nSamples; ++i)
                updateSum(samples[i]);
            return getRms();
        }

    private:
        std::vector<float> buffer_;
        float sum_{0.0f};
        std::size_t index_{0};
    };
}