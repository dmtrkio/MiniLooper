#pragma once

#include <tuple>

#include "processor_concept.h"

namespace dsp::processors {
    template <Processor... Processors>
    struct ProcessorChain
    {
        std::tuple<Processors...> chain;

        constexpr ProcessorChain() = default;

        constexpr explicit ProcessorChain(Processors... ps)
            : chain(std::move(ps)...) {}

        void prepare(float sampleRate)
        {
            std::apply([sampleRate](auto&... p) {
                (p.prepare(sampleRate), ...);
            }, chain);
        }

        void process(float* const* data, unsigned int nFrames)
        {
            std::apply([data, nFrames](auto&... p) {
                (p.process(data, nFrames), ...);
            }, chain);
        }
    };

    template <Processor... Ps>
    ProcessorChain(Ps...) -> ProcessorChain<Ps...>;
}