#pragma once

#include <cassert>
#include <array>
#include <cmath>
#include <vector>
#include <algorithm>

#include "processor_chain.h"

namespace dsp::processors {
    template <Processor... Processors>
    struct DryWetProcessor
    {
        ProcessorChain<Processors...> processors;

        explicit DryWetProcessor(std::size_t maxFramesInBuffer = 8096)
        {
            for (auto& buffer : buffers_) {
                buffer.resize(maxFramesInBuffer, 0.0f);
            }
        }

        void setDryWet(float dryWet) noexcept
        {
            dryWet_ = std::clamp(dryWet, 0.0f, 1.0f);
        }

        float getDryWet() const noexcept { return dryWet_; }

        void prepare(float sampleRate)
        {
            processors.prepare(sampleRate);
        }

        void process(float* const* data, unsigned int nFrames)
        {
            assert(nFrames <= buffers_[0].size());

            for (std::size_t channel{}; channel < 2; ++channel) {
                std::copy(data[channel], data[channel] + nFrames, buffers_[channel].begin());
            }

            float const* bufferPtrs[2] = {buffers_[0].data(), buffers_[1].data()};
            processors.process(bufferPtrs, nFrames);

            for (std::size_t channel{}; channel < 2; ++channel) {
                float *const output = data[channel];
                const float *processed = buffers_[channel].data();
                for (std::size_t frame{}; frame < nFrames; ++frame) {
                    output[frame] = std::lerp(output[frame], processed[frame], dryWet_);
                }
            }
        }

    private:
        float dryWet_ = 0.5f;
        std::array<std::vector<float>, 2> buffers_;
    };
}