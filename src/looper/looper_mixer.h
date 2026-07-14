#pragma once

#include <vector>
#include <memory>

#include "dsp/effects/effect_base.h"
#include "dsp/parameter/parameter_tree.h"

namespace ml::looper {
    class Mixer
    {
    public:
        explicit Mixer(const unsigned int nChannels);

        ~Mixer();
        Mixer(Mixer&&) noexcept;
        Mixer& operator=(Mixer&&) noexcept;
        Mixer(const Mixer&) = delete;
        Mixer& operator=(const Mixer&) = delete;

        void prepare(float sampleRate);
        void process(float *const *data, const unsigned int nFrames);
        dsp::parameter::ParameterTree getParameterTree() const noexcept;
        std::pair<float*, float*> getChannelBuffers(const int index);
        std::pair<float, float> getLevel(const int index);

    private:
        void applyParams();

        struct Channel;
        std::vector<std::unique_ptr<Channel>> channels_;

        dsp::parameter::ParameterTree paramTree_;
        std::unique_ptr<dsp::effects::EffectBase> outputFx_;
    };
}