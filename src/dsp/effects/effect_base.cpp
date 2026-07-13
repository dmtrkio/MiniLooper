#include "effect_base.h"
#include "dsp/parameter/parameter_tree.h"

#include <cassert>

namespace ml::dsp::effects {
    EffectBase::EffectBase(const std::string& name)
        : pt_(name)
    {
        pt_.addParameter(Param::makeBoolean("Enabled", false));
        enabledParam_.referTo(pt_["Enabled"].asParameterUnsafe());
    }

    void EffectBase::rename(std::string_view newName)
    {
        pt_.setName(newName);
    }

    void EffectBase::prepare(float sampleRate)
    {
        for (auto& step : processingSteps_) {
            step->prepare(sampleRate);
        }

        prepareInner(sampleRate);
    }

    void EffectBase::process(float *const *data, unsigned int nFrames) noexcept
    {
        if (!enabledParam_.get()) {
            // If the effect was disabled, reset its state
            if (prevEnabled_) {
                reset();
                prevEnabled_ = false;
            }
            return;
        }
        prevEnabled_ = true;

        for (auto& step : processingSteps_) {
            step->process(data, nFrames);
        }

        processInner(data, nFrames);
    }

    EffectBase::ParamTree EffectBase::getParameterTree() const noexcept
    {
        return pt_;
    }

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

    void EffectBase::setEnabled(bool enabled) noexcept
    {
        enabledParam_.set(enabled);
    }

    void EffectBase::reset() noexcept
    {
        for (auto& step : processingSteps_) {
            step->reset();
        }
        resetInner();
    }

    void EffectBase::prepareInner(float) {}
    void EffectBase::resetInner() noexcept {}

    void EffectBase::attachParameters(const std::vector<ParamTree>& params)
    {
        for (const auto& paramTree : params) {
            if (paramTree.isValid()) {
                pt_.addSubTree(paramTree);
            }
        }
    }

    EffectBase& EffectBase::addProcessingStep(std::unique_ptr<EffectBase> effect)
    {
        processingSteps_.emplace_back(std::move(effect));
        const auto paramTree = processingSteps_.back()->getParameterTree();
        attachParameters({std::move(paramTree)});
        return *processingSteps_.back();
    }
}