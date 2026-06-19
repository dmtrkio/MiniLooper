#pragma once

#include <cassert>
#include <cmath>
#include <vector>
#include <algorithm>

namespace ml::dsp {
    class FractionalDelayLine
    {
    public:
        explicit FractionalDelayLine()
        {
            const auto defaultMaxDelaySamples = 16000.0f;
            prepare(defaultMaxDelaySamples);
        }

        void prepare(const float maxDelaySamples)
        {
            const std::size_t maxDelaySamplesInt = static_cast<int>(std::ceil(maxDelaySamples));
            assert(maxDelaySamplesInt > 0);
            buffer_.assign(maxDelaySamplesInt + 1, 0.0f);
            writeIndex_ = 0;
            delayInt_ = 0;
            delayFrac_ = 0.0f;
        }

        int getMaxDelaySamples() const noexcept { return static_cast<int>(buffer_.size()); }

        void setDelay(float delaySamples)
        {
            delaySamples = std::max(0.0f, std::min(delaySamples, static_cast<float>(getMaxDelaySamples() - 1)));
            delayInt_ = static_cast<int>(delaySamples);
            delayFrac_ = delaySamples - static_cast<float>(delayInt_);
        }

        float process(const float input) noexcept
        {
            buffer_[writeIndex_] = input;

            const auto size = getMaxDelaySamples();

            int readIndex = writeIndex_ - delayInt_;
            if (readIndex < 0) readIndex += size;

            const float s0 = buffer_[readIndex];
            int nextIndex = readIndex + 1;
            if (nextIndex >= size) nextIndex = 0;
            const float s1 = buffer_[nextIndex];

            const float output = s0 + delayFrac_ * (s1 - s0);

            if (++writeIndex_ >= size) writeIndex_ = 0;

            return output;
        }

        void clear()
        {
            std::ranges::fill(buffer_, 0.0f);
            writeIndex_ = 0;
        }

    private:
        std::vector<float> buffer_{};
        int writeIndex_{0};
        int delayInt_{0};
        float delayFrac_{0.0f};
    };
}