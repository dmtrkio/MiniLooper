#pragma once

#include <algorithm>
#include <array>
#include <vector>
#include <cassert>
#include <ranges>

namespace looper {
    static constexpr int kNoInput = -1;

    class SourceChannel
    {
    public:
        void prepare(unsigned int nInputBuffers, unsigned int maxFramesInBuffer, unsigned int sampleRate)
        {
            nInputBuffers_ = nInputBuffers;

            for (auto& buf : buffers_) {
                buf.assign(maxFramesInBuffer, 0.0f);
            }

            for (auto& input : inputs_) {
                if (input < 0 || input >= nInputBuffers_) {
                    input = kNoInput;
                }
            }
        }

        void process(float *const *in, float *const *out, unsigned int nFrames) noexcept
        {
            assert(nFrames <= buffers_[0].size());

            if (stereo_) {
                for (auto [input, gain, buf] : std::views::zip(inputs_, inputGains_, buffers_)) {
                    if (input == kNoInput) {
                        std::fill_n(buf.begin(), nFrames, 0.0f);
                        continue;
                    }

                    const float* inputBuffer = in[static_cast<std::size_t>(input)];
                    assert(inputBuffer != nullptr);

                    for (auto frame{0u}; frame < nFrames; ++frame) {
                        buf[frame] = inputBuffer[frame] * gain;
                    }
                }
            } else {
                const auto input = inputs_[0];
                const auto gain = inputGains_[0];
                if (input == kNoInput) {
                    for (auto& buf : buffers_) {
                        std::fill_n(buf.begin(), nFrames, 0.0f);
                    }
                } else {
                    const float* inputBuffer = in[static_cast<std::size_t>(input)];
                    assert(inputBuffer != nullptr);

                    for (auto frame{0u}; frame < nFrames; ++frame) {
                        buffers_[0][frame] = inputBuffer[frame] * gain;
                    }

                    for (std::size_t i = 1; i < buffers_.size(); ++i) {
                        std::copy_n(buffers_[0].data(), nFrames, buffers_[i].data());
                    }
                }
            }
            
            processInternal();

            for (auto ch{0u}; ch < 2; ++ch) {
                for (auto frame{0u}; frame < nFrames; ++frame) {
                    out[ch][frame] = buffers_[ch][frame];
                }
            }
        }

    private:
        void processInternal()
        {

        }

        unsigned int nInputBuffers_{0};
        bool stereo_{false};
        std::array<int, 2> inputs_{kNoInput, kNoInput};
        std::array<float, 2> inputGains_{1.0f, 1.0f};
        std::array<std::vector<float>, 2> buffers_{};
    };

    class SourceMixer
    {
    public:
        void prepare(unsigned int nInputs, unsigned int maxFramesInBuffer, unsigned int sampleRate)
        {
            for (auto& ch : channels_) {
                ch.prepare(nInputs, maxFramesInBuffer, sampleRate);
            }
        }

        void process(float *const *in, float *const *out, unsigned int nFrames) noexcept
        {
            for (auto& ch : channels_) {
                ch.process(in, out, nFrames);
            }
        }

    private:
        std::array<SourceChannel, 2> channels_{};
    };
}