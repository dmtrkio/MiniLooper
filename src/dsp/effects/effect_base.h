#pragma once

#include <string>

#include "rt_sanitizer.h"
#include "dsp/parameter/parameter_tree.h"
#include "dsp/parameter/parameter_view.h"

namespace ml::dsp::effects {
    struct EffectBase
    {
        using Param = parameter::Parameter;
        using ParamTree = parameter::ParameterTree;

        EffectBase(const std::string& name);
        virtual ~EffectBase() = default;

        virtual void prepare(float sampleRate);
        void process(float *const *data, unsigned int nFrames) RT_SAN noexcept;

        [[nodiscard]] ParamTree getParameterTree() const noexcept;

        [[nodiscard]] json getSettingsAsJson() const;
        bool loadSettingsFromJson(const json& j) noexcept;

        void setEnabled(bool enabled) noexcept;

        // implement reset of dsp state in the derived class if needed, should be realtime safe (no allocations, locks etc)
        virtual void reset() RT_SAN noexcept;

    protected:
        // implement actual processing in the derived class
        virtual void processInner(float *const *data, unsigned int nFrames) RT_SAN noexcept = 0;

        // call this in the derived class constructor to attach internal parameters of the derived class to the base class parameter tree
        void attachParameters(const std::vector<ParamTree>& params) noexcept;

    private:
        ParamTree pt_;
        parameter::ParameterView<bool> enabledParam_;
        bool prevEnabled_{false};
    };
}