#pragma once

#include "../parameter/parameter_tree.h"

namespace dsp::effects {
    struct EffectBase
    {
        virtual ~EffectBase() = default;
        virtual void prepare(float sampleRate) = 0;
        virtual void process(float *const *data, unsigned int nFrames) = 0;
        virtual parameter::ParameterTree getParameterTree() = 0;
    };
}