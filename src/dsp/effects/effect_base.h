#pragma once

#include "../parameter/parameter_tree.h"
#include "rt_sanitizer.h"

namespace ml::dsp::effects {
    struct EffectBase
    {
        virtual ~EffectBase() = default;
        virtual void prepare(float sampleRate) = 0;
        virtual void process(float *const *data, unsigned int nFrames) RT_SAN = 0;
        virtual parameter::ParameterTree getParameterTree() const = 0;

        [[nodiscard]] json getSettingsAsJson() const;
        bool loadSettingsFromJson(const json& j) noexcept;
    };
}