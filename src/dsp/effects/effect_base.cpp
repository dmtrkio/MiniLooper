#include "effect_base.h"

namespace dsp::effects {
    json EffectBase::getSettingsAsJson() const
    {
        auto tree = getParameterTree();
        if (tree.isValid()) {
            return tree.toJson();
        }
        return nullptr;
    }

    bool EffectBase::loadSettingsFromJson(const json& j) noexcept
    {
        auto tree = getParameterTree();
        if (tree.isValid()) {
            return tree.copyParameterValuesFromJson(j);
        }
        return false;
    }
}