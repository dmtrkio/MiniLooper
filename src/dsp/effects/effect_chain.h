#pragma once

#include "effect_base.h"
#include "dsp/processors/processor_chain.h"

namespace ml::dsp::effects {
    template <typename... Effects>
    requires (std::derived_from<Effects, EffectBase> && ...)
    class EffectChain final : public EffectBase
    {
    public:
        EffectChain(const std::string& name) : EffectBase(name)
        {
            std::vector<ParamTree> params;

            std::apply([&](auto&... e) {
                (params.push_back(e.getParameterTree()), ...);
            }, chain_.chain);

            attachParameters(params);
        }

    protected:
        void prepareInner(float sampleRate) override
        {
            chain_.prepare(sampleRate);
        }

        void processInner(float *const *data, const unsigned int nFrames) noexcept override
        {
            chain_.process(data, nFrames);
        }

        void resetInner() noexcept override
        {
            std::apply([](auto&... e) {
                (e.reset(), ...);
            }, chain_.chain);
        }

    private:
        processors::ProcessorChain<Effects...> chain_;
    };
}