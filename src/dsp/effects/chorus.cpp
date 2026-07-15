#include "chorus.h"

namespace ml::dsp::effects {
    Chorus::Chorus() : EffectBase("Chorus")
    {
        std::vector<ParamTree> params = {
            ParamTree{Param::makeFloat("Rate", 1.0f, {0.2f, 3.0f})},
            ParamTree{Param::makeFloat("Depth", 0.5f, {0.0f, 1.0f})},
            ParamTree{Param::makeFloat("Feedback", 0.0f, {0.0f, 1.0f})},
            ParamTree{Param::makeFloat("Mix", 0.5f, {0.0f, 1.0f})},
        };

        attachParameters(params);

        rateParam_.referTo(params[0].asParameterUnsafe());
        depthParam_.referTo(params[1].asParameterUnsafe());
        feedbackParam_.referTo(params[2].asParameterUnsafe());
        mixParam_.referTo(params[3].asParameterUnsafe());
    }

    void Chorus::prepareInner(float sampleRate)
    {
        static constexpr float kSmoothingMs = 1.0f;
        const auto smoothFrames = kSmoothingMs * sampleRate * 0.001f;

        sampleRate_ = sampleRate;

        rate_.setSmoothingFrames(smoothFrames);
        depth_.setSmoothingFrames(smoothFrames);
        feedback_.setSmoothingFrames(smoothFrames);
        mix_.setSmoothingFrames(smoothFrames);

        rate_.init(rateParam_.get());
        depth_.init(depthParam_.get());
        feedback_.init(feedbackParam_.get());
        mix_.init(mixParam_.get());

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

    void Chorus::processInner(float *const *data, const unsigned int nFrames) noexcept
    {
        rate_.setTarget(rateParam_.get());
        depth_.setTarget(depthParam_.get());
        feedback_.setTarget(feedbackParam_.get());
        mix_.setTarget(mixParam_.get());

        for (std::size_t i{}; i < nFrames; ++i) {
            const auto input = std::make_pair(data[0][i], data[1][i]);
            const auto processed = processFrame(input);
            const auto mix = mix_();
            data[0][i] = std::lerp(input.first, processed.first, mix);
            data[1][i] = std::lerp(input.second, processed.second, mix);
        }
    }

    std::pair<float, float> Chorus::processFrame(std::pair<float, float> input) noexcept
    {
        auto output = std::make_pair(0.0f, 0.0f);
        const auto rate = rate_();
        const auto depth = depth_();
        const auto maxFeedback = 0.8f;
        const auto feedback = std::lerp(0.0f, maxFeedback, feedback_());

        for (auto& v : voices_) {
            const auto voiceOutput = v.processFrame(input, rate, depth, feedback, sampleRate_);
            output.first += voiceOutput.first;
            output.second += voiceOutput.second;
        }

        const auto scale = 1.0f / static_cast<float>(voices_.size());
        output.first *= scale;
        output.second *= scale;

        return output;
    }

    void Chorus::Voice::setup(float lfoPhaseOffset, float lfoRateOffset, float minDelayMs, float maxDelayMs, float sampleRate, bool flip) noexcept
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

        for (std::size_t i{}; i < delayLines.size(); ++i) {
            delayLines[i].prepare(maxDelays[i]);
        }
    }

    std::pair<float, float> Chorus::Voice::processFrame(std::pair<float, float> input, float rate, float depth, float feedback, float sampleRate) noexcept
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

        return prevInput = {
            delayLines[0].process(leftInput + prevInput.first * feedback),
            delayLines[1].process(rightInput + prevInput.second * feedback)
        };
    }
}