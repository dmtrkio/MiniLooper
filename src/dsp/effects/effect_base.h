#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <memory>

#include "rt_sanitizer.h"
#include "dsp/parameter/parameter_tree.h"
#include "dsp/parameter/parameter_view.h"

namespace ml::dsp::effects {
    // Base Class for stereo DSP effects
    struct EffectBase
    {
        using Param = parameter::Parameter;
        using ParamTree = parameter::ParameterTree;

        EffectBase(const std::string& name);
        virtual ~EffectBase() = default;

        void rename(std::string_view newName);

        void prepare(float sampleRate);
        void process(float *const *data, unsigned int nFrames) noexcept RT_SAN;

        [[nodiscard]] ParamTree getParameterTree() const noexcept;

        [[nodiscard]] json getSettingsAsJson() const;
        bool loadSettingsFromJson(const json& j) noexcept;

        void setEnabled(bool enabled) noexcept;

        void reset() noexcept RT_SAN;

    protected:
        // implement preparation for audio processing
        virtual void prepareInner(float sampleRate);

        // implement actual processing in the derived class
        virtual void processInner(float *const *data, unsigned int nFrames) noexcept RT_SAN = 0;

        // implement reset of dsp state in the derived class if needed, should be realtime safe (no allocations, locks etc)
        virtual void resetInner() noexcept RT_SAN;

        // call this in the derived class constructor to attach internal parameters of the derived class to the base class parameter tree
        void attachParameters(const std::vector<ParamTree>& params);

        // Allow for easy creation of sequence of effects.
        // Callbacks will be called in order of creation.
        // Call in the constructor of derived class
        EffectBase& addProcessingStep(std::unique_ptr<EffectBase> effect);

    private:
        std::vector<std::unique_ptr<EffectBase>> processingSteps_{};
        ParamTree pt_;
        parameter::ParameterView<bool> enabledParam_;
        bool prevEnabled_{false};
    };
}