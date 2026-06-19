#pragma once

#include <concepts>

namespace ml::dsp::processors {
    template <typename T>
    concept Processor = requires(T p, float sampleRate, float* const* data, unsigned int nFrames) {
        { p.prepare(sampleRate) } -> std::same_as<void>;
        { p.process(data, nFrames) } -> std::same_as<void>;
    };
}