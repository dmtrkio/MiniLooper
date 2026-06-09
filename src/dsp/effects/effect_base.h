#pragma once

#include "../parameter/parameter_tree.h"
#include "rt_sanitizer.h"

namespace dsp::effects {
    struct EffectBase
    {
        virtual ~EffectBase() = default;
        virtual void prepare(float sampleRate) = 0;
        virtual void process(float *const *data, unsigned int nFrames) RT_SAN = 0;
        virtual parameter::ParameterTree getParameterTree() const = 0;

        [[nodiscard]] json getSettingsAsJson() const
        {
            auto tree = getParameterTree();
            if (tree.isValid()) {
                return tree.toJson();
            }
            return nullptr;
        }

        bool loadSettingsFromJson(const json& j) noexcept
        {
            auto tree = getParameterTree();
            if (tree.isValid()) {
                return tree.copyParameterValuesFromJson(j);
            }
            return false;
        }
    };
}