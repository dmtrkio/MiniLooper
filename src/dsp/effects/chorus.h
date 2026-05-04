#pragma once

#include <array>

#include "dsp/dsp.h"
#include "dsp/effects/effect_base.h"
#include "dsp/oscillator.h"
#include "dsp/delay_line.h"

namespace dsp::effects {
    class Chorus final : public EffectBase
    {
    public:
        Chorus()
        {
            rate_.init(paramTree_["Rate"].asParameterUnsafe().get<float>());
            depth_.init(paramTree_["Depth"].asParameterUnsafe().get<float>());
            mix_.init(paramTree_["Mix"].asParameterUnsafe().get<float>());
        }

        void prepare(float sampleRate) override
        {
            static constexpr float kSmoothingMs = 1.0f;
            const auto smoothFrames = kSmoothingMs * sampleRate * 0.001f;

            sampleRate_ = sampleRate;

            rate_.setSmoothingFrames(smoothFrames);
            depth_.setSmoothingFrames(smoothFrames);
            mix_.setSmoothingFrames(smoothFrames);

            float lfoPhaseOffset = 0.0f;
            float lfoRateOffset = 0.0f;
            float minDelayMs = 5.0f;
            float maxDelayMs = 15.0f;
            bool flip = false;

            for (auto& v : voices_) {
                lfoPhaseOffset += 0.1f;
                lfoRateOffset += 0.1f;
                minDelayMs += 1.2f;
                maxDelayMs += 1.2f;
                flip = !flip;
                v.setup(
                    lfoPhaseOffset,
                    lfoRateOffset,
                    minDelayMs,
                    maxDelayMs,
                    sampleRate,
                    flip
                );
            }
        }

        void process(float *const *data, const unsigned int nFrames) override
        {
            applyParams();

            for (std::size_t i{}; i < nFrames; ++i) {
                const auto input = std::make_pair(data[0][i], data[1][i]);
                const auto processed = processFrame(input);
                const auto mix = mix_();
                data[0][i] = std::lerp(input.first, processed.first, mix);
                data[1][i] = std::lerp(input.second, processed.second, mix);
            }
        }

        dsp::parameter::ParameterTree getParameterTree() const override
        {
            return paramTree_;
        }
        
    private:
        void applyParams()
        {
            rate_.setTarget(paramTree_["Rate"].asParameterUnsafe().get<float>());
            depth_.setTarget(paramTree_["Depth"].asParameterUnsafe().get<float>());
            mix_.setTarget(paramTree_["Mix"].asParameterUnsafe().get<float>());
        }

        std::pair<float, float> processFrame(std::pair<float, float> input) noexcept
        {
            auto output = std::make_pair(0.0f, 0.0f);
            const auto rate = rate_();
            const auto depth = depth_();

            for (auto& v : voices_) {
                const auto voiceOutput = v.processFrame(input, rate, depth, sampleRate_);
                output.first += voiceOutput.first;
                output.second += voiceOutput.second;
            }

            const auto scale = 1.0f / static_cast<float>(voices_.size());
            output.first *= scale;
            output.second *= scale;

            return output;
        }

        struct Voice
        {
            float rateOffset;
            Oscillator lfo;
            std::array<float, 2> minDelays, maxDelays;
            std::array<FractionalDelayLine, 2> delayLines;

            void setup(float lfoPhaseOffset, float lfoRateOffset, float minDelayMs, float maxDelayMs, float sampleRate, bool flip) noexcept
            {
                lfo.phaseOffset(lfoPhaseOffset);
                rateOffset = lfoRateOffset;

                const float minDelay = (minDelayMs * sampleRate) * 0.001f;
                const float maxDelay = (maxDelayMs * sampleRate) * 0.001f;
                const float stereoDiff = 0.001f * sampleRate * 0.5f;

                minDelays[0] = minDelay - stereoDiff;
                minDelays[1] = minDelay + stereoDiff;
                maxDelays[0] = maxDelay - stereoDiff;
                maxDelays[1] = maxDelay + stereoDiff;

                if (flip) {
                    std::swap(minDelays[0], minDelays[1]);
                    std::swap(maxDelays[0], maxDelays[1]);
                }
            }

            std::pair<float, float> processFrame(std::pair<float, float> input, float rate, float depth, float sampleRate) noexcept
            {
                lfo.setFrequency(rate + rateOffset, sampleRate);
                const auto lfoValue = lfo.sine() * 0.5f + 1.0f;
                lfo.tick();

                const float delayTimes[2] = {
                    std::lerp(minDelays[0], maxDelays[0], lfoValue * depth),
                    std::lerp(minDelays[1], maxDelays[1], lfoValue * depth)
                };

                delayLines[0].setDelay(delayTimes[0]);
                delayLines[1].setDelay(delayTimes[1]);

                constexpr float stereoBleed = 0.2f;
                const auto leftInput = std::lerp(input.first, input.second, stereoBleed);
                const auto rightInput = std::lerp(input.second, input.first, stereoBleed);

                return {
                    delayLines[0].process(leftInput),
                    delayLines[1].process(rightInput)
                };
            }
        };

        float sampleRate_{44100.0f};
        FloatSmoother rate_;
        FloatSmoother depth_;
        FloatSmoother mix_;

        std::array<Voice, 4> voices_;

        using Param = parameter::Parameter;
        using ParamTree = parameter::ParameterTree;

        ParamTree paramTree_{"Chorus",
            {
                Param::makeFloat("Rate", 1.0f, {0.2f, 3.0f}),
                Param::makeFloat("Depth", 0.5f, {0.0f, 1.0f}),
                Param::makeFloat("Mix", 0.0f, {0.0f, 1.0f})
            }
        };
    };
}