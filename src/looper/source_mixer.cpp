#include "source_mixer.h"

#include <algorithm>
#include <cassert>

#include "dsp/dsp.h"

namespace looper {
    SourceChannel::SourceChannel(const std::string name) : paramTree_(buildParamTree(name)) {}

    void SourceChannel::prepare(unsigned int nInputBuffers, unsigned int maxFramesInBuffer, float sampleRate)
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

        processingChain_.prepare(sampleRate);
        levelMeter_.prepare(sampleRate);
    }

    void SourceChannel::processAdding(const float *const *in, float *const *out, unsigned int nFrames) noexcept
    {
        assert(nFrames <= buffers_[0].size());

        applyParams();

        for (auto& buf : buffers_) {
            std::fill_n(buf.begin(), nFrames, 0.0f);
        }

        if (stereo_) {
            dsp::staticFor<2>([&](auto ch) {
                const auto input = inputs_[ch];
                const auto gain = inputGains_[ch];
                auto& buf = buffers_[ch];

                if (input == kNoInput) {
                    return;
                }

                const float* inputBuffer = in[static_cast<std::size_t>(input)];
                assert(inputBuffer != nullptr);

                for (auto frame{0u}; frame < nFrames; ++frame) {
                    buf[frame] = inputBuffer[frame] * gain;
                }
            });
        } else {
            dsp::staticFor<2>([&](auto ch) {
                const auto input = inputs_[ch];
                const auto gain = inputGains_[ch];

                if (input == kNoInput) {
                    return;
                }

                const float* inputBuffer = in[static_cast<std::size_t>(input)];
                assert(inputBuffer != nullptr);

                for (auto frame{0u}; frame < nFrames; ++frame) {
                    const auto inputSample = inputBuffer[frame] * gain;
                    for (auto& buf : buffers_) {
                        buf[frame] += inputSample;
                    }
                }
            });
        }
        
        if (!skipProcessing_) {
            processInternal(nFrames);
        }

        dsp::staticFor<2>([&](auto ch) {
            for (std::size_t frame{}; frame < nFrames; ++frame) {
                out[ch][frame] += buffers_[ch][frame];
            }
        });
    }

    dsp::parameter::ParameterTree SourceChannel::getParameterTree() const noexcept { return paramTree_; }
    std::pair<float, float> SourceChannel::getLevel() const noexcept { return levelMeter_.getLevel(); }

    void SourceChannel::skipInternalProcessing(bool shouldSkip) noexcept
    {
        skipProcessing_ = shouldSkip;
    }

    SourceChannel::ParamTree SourceChannel::buildParamTree(const std::string& name) const
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
            processingChain_.getParameterTree(),
        }};
    }

    void SourceChannel::applyParams()
    {
        assert(inputs_.size() == 2);

        const auto getInputParams = [&](std::size_t inputIndex) -> std::pair<int, float> {
            const auto inputName = (inputIndex == 0) ? "Input1" : "Input2";
            const auto subtree = paramTree_[inputName];
            const auto inputParam = subtree["Source"];
            const auto gainParam = subtree["GainDb"];

            const auto input = inputParam.asParameterUnsafe().get<int>();
            const auto gainDb = gainParam.asParameterUnsafe().get<float>();

            return {
                (input >= 0 && input < nInputBuffers_) ? input : kNoInput,
                dsp::dBtoLinear(gainDb)
            };
        };

        for (std::size_t i = 0; i < inputs_.size(); ++i) {
            std::tie(inputs_[i], inputGains_[i]) = getInputParams(i);
        }

        stereo_ = paramTree_["Stereo"].asParameterUnsafe().get<bool>();
    }

    void SourceChannel::processInternal(unsigned int nFrames)
    {
        if (std::ranges::all_of(inputs_, [](int input) { return input == kNoInput; })) {
            return;
        }

        float *channelData[2] = {buffers_[0].data(), buffers_[1].data()};
        processingChain_.process(channelData, nFrames);
        levelMeter_(channelData[0], channelData[1], nFrames);
    }

    void SourceMixer::prepare(unsigned int nInputs, unsigned int maxFramesInBuffer, float sampleRate)
    {
        for (auto& ch : channels_) {
            ch.prepare(nInputs, maxFramesInBuffer, sampleRate);
        }
    }

    void SourceMixer::process(const float *const *in, float *const *out, unsigned int nFrames) noexcept
    {
        dsp::staticFor<2>([&](auto channel) {
            std::fill_n(out[channel], nFrames, 0.0f);
        });

        for (auto& ch : channels_) {
            ch.processAdding(in, out, nFrames);
        }
    }

    dsp::parameter::ParameterTree SourceMixer::getParameterTree() const noexcept
    {
        return paramTree_;
    }

    std::array<std::pair<float, float>, SourceMixer::kNumChannels> SourceMixer::getLevels() const noexcept
    {
        std::array<std::pair<float, float>, kNumChannels> levels{};
        for (std::size_t i = 0; i < kNumChannels; ++i) {
            levels[i] = channels_[i].getLevel();
        }
        return levels;
    }

    void SourceMixer::skipInternalProcessing(bool shouldSkip) noexcept
    {
        for (auto& ch : channels_) {
            ch.skipInternalProcessing(shouldSkip);
        }
    }
}