#pragma once

#include <array>
#include <vector>

#include "dsp/parameter/parameter_tree.h"
#include "dsp/level_meter.h"
#include "processing_chain.h"

namespace looper {
    class SourceChannel
    {
    public:
        static constexpr int kNoInput = -1;

        SourceChannel(const std::string name);

        void prepare(unsigned int nInputBuffers, unsigned int maxFramesInBuffer, unsigned int sampleRate);
        void processAdding(const float *const *in, float *const *out, unsigned int nFrames) noexcept;

        [[nodiscard]] dsp::parameter::ParameterTree getParameterTree() const noexcept;
        [[nodiscard]] std::pair<float, float> getLevel() const noexcept;
        
        // for testing purposes
        void skipInternalProcessing(bool shouldSkip) noexcept;

    private:
        using Param = dsp::parameter::Parameter;
        using ParamTree = dsp::parameter::ParameterTree;

        [[nodiscard]] ParamTree buildParamTree(const std::string& name) const;
        void applyParams();
        void processInternal(unsigned int nFrames);

        bool skipProcessing_{false};
        int nInputBuffers_{0};
        bool stereo_{false};
        std::array<int, 2> inputs_{kNoInput, kNoInput};
        std::array<float, 2> inputGains_{1.0f, 1.0f};
        std::array<std::vector<float>, 2> buffers_{};
        ProcessingChain processingChain_;
        ParamTree paramTree_;
        dsp::LevelMeter levelMeter_;
    };

    class SourceMixer
    {
    public:
        static constexpr std::size_t kNumChannels = 2;

        void prepare(unsigned int nInputs, unsigned int maxFramesInBuffer, unsigned int sampleRate);
        void process(const float *const *in, float *const *out, unsigned int nFrames) noexcept;

        [[nodiscard]] dsp::parameter::ParameterTree getParameterTree() const noexcept;
        [[nodiscard]] std::array<std::pair<float, float>, kNumChannels> getLevels() const noexcept;

    private:
        std::array<SourceChannel, kNumChannels> channels_{
            SourceChannel("SourceChannel1"),
            SourceChannel("SourceChannel2"),
        };

        dsp::parameter::ParameterTree paramTree_{"SourceMixer", {
            channels_[0].getParameterTree(),
            channels_[1].getParameterTree(),
        }};
    };
}