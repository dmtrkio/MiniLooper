#pragma once

#include <algorithm>
#include <array>
#include <vector>
#include <cassert>
#include <ranges>

#include "dsp/parameter/parameter_tree.h"

namespace looper {
    static constexpr int kNoInput = -1;

    class SourceChannel
    {
    public:
        SourceChannel(const std::string name) : paramTree(buildParamTree(name)) {}

        void prepare(unsigned int nInputBuffers, unsigned int maxFramesInBuffer, unsigned int sampleRate)
        {
            nInputBuffers_ = static_cast<int>(nInputBuffers);

            for (auto& buf : buffers_) {
                buf.assign(maxFramesInBuffer, 0.0f);
            }

            applyParams();

            for (auto& input : inputs_) {
                if (input < 0 || input >= nInputBuffers_) {
                    input = kNoInput;
                }
            }
        }

        void processAdding(const float *const *in, float *const *out, unsigned int nFrames) noexcept
        {
            applyParams();

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
                    out[ch][frame] += buffers_[ch][frame];
                }
            }
        }

        using Param = dsp::parameter::Parameter;
        using ParamTree = dsp::parameter::ParameterTree;

        ParamTree paramTree;

    private:
        static ParamTree buildParamTree(const std::string& name)
        {
            return {name, {
                ParamTree{"Input1", {
                    Param::makeInteger("Source", kNoInput, {kNoInput, 64}),
                    Param::makeFloat("GainDb", 0.0f, {-60.0f, 12.0f}),
                }},
                ParamTree{"Input2", {
                    Param::makeInteger("Source", kNoInput, {kNoInput, 64}),
                    Param::makeFloat("GainDb", 0.0f, {-60.0f, 12.0f}),
                }},
                ParamTree{Param::makeBoolean("Stereo", false)},
            }};
        }

        void applyParams()
        {
            assert(inputs_.size() == 2);

            const auto getInputParams = [&](std::size_t inputIndex) -> std::pair<int, float> {
                const auto inputName = (inputIndex == 0) ? "Input1" : "Input2";
                const auto subtree = paramTree[inputName];
                const auto inputParam = subtree["Source"];
                const auto gainParam = subtree["GainDb"];

                const auto input = inputParam.asParameterUnsafe().get<int>();
                const auto gainDb = gainParam.asParameterUnsafe().get<float>();

                return {input, dsp::dBtoLinear(gainDb)};
            };

            for (std::size_t i = 0; i < inputs_.size(); ++i) {
                std::tie(inputs_[i], inputGains_[i]) = getInputParams(i);
            }

            stereo_ = paramTree["Stereo"].asParameterUnsafe().get<bool>();
        }

        void processInternal()
        {
            // TODO
        }

        int nInputBuffers_{0};
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

        void process(const float *const *in, float *const *out, unsigned int nFrames) noexcept
        {
            for (auto ch{0u}; ch < 2; ++ch) {
                std::fill_n(out[ch], nFrames, 0.0f);
            }

            for (auto& ch : channels_) {
                ch.processAdding(in, out, nFrames);
            }
        }

        dsp::parameter::ParameterTree getParameterTree() const noexcept
        {
            return paramTree_;
        }

    private:
        static constexpr std::size_t kNumChannels = 2;
        std::array<SourceChannel, kNumChannels> channels_{
            SourceChannel("SourceChannel1"),
            SourceChannel("SourceChannel2"),
        };

        dsp::parameter::ParameterTree paramTree_{"SourceMixer", {
            channels_[0].paramTree,
            channels_[1].paramTree,
        }};
    };
}