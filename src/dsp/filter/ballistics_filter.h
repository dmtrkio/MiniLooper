#pragma once

namespace ml::dsp::filter {
    class BallisticsFilter
    {
    public:
        void prepare(const float sampleRate, const float attackMs, const float releaseMs, const float initial = 0.0f) noexcept;
        float process(const float x) noexcept;
        [[nodiscard]] float getState() const noexcept;

    private:
        float attack_{0.0f};
        float release_{0.0f};
        float state_{0.0f};
    };
}