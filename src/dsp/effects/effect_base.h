#pragma once

#include <string_view>
#include <vector>
#include <memory>

#include "rt_sanitizer.h"
#include "dsp/parameter/parameter_tree.h"
#include "dsp/parameter/parameter_view.h"

namespace ml::dsp::effects {
    struct EffectBase;

    template<typename... Args>
    std::unique_ptr<EffectBase> createEffectSequence(
        std::string_view name,
        bool enabled,
        Args&&... steps
    );

    // Base Class for stereo DSP effects
    struct EffectBase
    {
        using Param = parameter::Parameter;
        using ParamTree = parameter::ParameterTree;

        explicit EffectBase(std::string_view name, bool enabled = false);
        explicit EffectBase(std::string_view name, std::vector<std::unique_ptr<EffectBase>> steps, bool enabled = false);

        virtual ~EffectBase() = default;
        EffectBase(const EffectBase&) = delete;
        EffectBase& operator=(const EffectBase&) = delete;
        EffectBase(EffectBase&&) noexcept = default;
        EffectBase& operator=(EffectBase&&) noexcept = default;

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
        virtual void processInner(float *const *data, unsigned int nFrames) noexcept RT_SAN;

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

    template<typename... Args>
    inline std::unique_ptr<EffectBase> createEffectSequence(
        std::string_view name,
        bool enabled,
        Args&&... steps
    )
    {
        std::vector<std::unique_ptr<EffectBase>> vec;
        vec.reserve(sizeof...(steps));
        (vec.emplace_back(std::forward<Args>(steps)), ...);
        
        return std::make_unique<EffectBase>(name, std::move(vec), enabled);
    }
}