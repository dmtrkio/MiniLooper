#include "reverb.h"

#include "dsp/delay_line.h"
#include "dsp/dsp.h"
#include "dsp/parameter/parameter_view.h"

namespace ml::dsp::effects {
    class SchroederAllPass : public FractionalDelayLine
    {
    public:
        void setGain(float gain) noexcept
        {
            gain_ = gain;
        }

        [[nodiscard]] float process(float input) noexcept
        {
            const float delayed = read();
            const float w = input - gain_ * delayed;
            write(w);
            const float output = w * gain_ + delayed;
            return output;
        }

    private:
        float gain_ = 0.7f;
    };

    class CombFilter : public FractionalDelayLine
    {
    public:
        void setFeedback(float feedback) noexcept
        {
            feedback_ = feedback;
        }

        [[nodiscard]] float process(float input) noexcept
        {
            return processWithFeedback(input, feedback_);
        }

    private:
        float feedback_ = 0;
    };

    class SchroederReverb final : public EffectBase
    {
    public:
        explicit SchroederReverb(bool enabled = false)
            : EffectBase("Schroeder Reverb", enabled)
        {
            std::vector<ParamTree> params = {
                ParamTree{Param::makeFloat("Mix", 0.5f, dsp::Range{0.0f, 1.0f})},
            };

            attachParameters(params);

            mixParam_.referTo(params[0].asParameterUnsafe());
        }

    protected:
        void prepareInner(float sampleRate) override
        {
            static constexpr float kSmoothingMs = 1.0f;
            const auto smoothFrames = kSmoothingMs * sampleRate * 0.001f;

            mix_.init(dsp::dBtoLinear(mixParam_.get()));
            mix_.setSmoothingFrames(smoothFrames);

            constexpr std::array<float, kAllPassCount> allPassDelays = { 347.0f, 113.0f, 37.0f };
            constexpr float allPassGain = 0.7f;

            constexpr std::array<float, kCombCount> combDelays = { 1687.0f, 1601.0f, 2053.0f, 2251.0f };
            constexpr std::array<float, kCombCount> combFeedback = {0.773f, 0.802f, 0.753f, 0.733f };

            const auto convertSr = [sampleRate](float x) {
                return x * (sampleRate / 25000.0f);
            };

            for (std::size_t i = 0; i < allPasses_.size(); ++i) {
                const float delay = convertSr(allPassDelays[i]);
                allPasses_[i].prepare(delay);
                allPasses_[i].setDelay(delay);
                allPasses_[i].setGain(allPassGain);
            }

            for (std::size_t i = 0; i < combs_.size(); ++i) {
                const float delay = convertSr(combDelays[i]);
                combs_[i].prepare(delay);
                combs_[i].setDelay(delay);
                combs_[i].setFeedback(combFeedback[i]);
            }
        }

        void processInner(float *const *data, unsigned int nFrames) noexcept override
        {
            mix_.setTarget(mixParam_.get());

            float* left = data[0];
            float* right = data[1];

            for (unsigned int n = 0; n < nFrames; ++n) {
                const float monoIn = (left[n] + right[n]) * 0.18f;

                float allpassOutput = monoIn;
                for (auto& ap : allPasses_) {
                    allpassOutput = ap.process(allpassOutput);
                }

                std::array<float, kCombCount> x;
                for (std::size_t i = 0; i < combs_.size(); ++i) {
                    x[i] = combs_[i].process(allpassOutput);
                }

                const float s1 = x[0] + x[2];
                const float s2 = x[1] + x[3];

                const float outA = s1 + s2;
                const float outB = -outA;
                (void)outB;
                const float outD = s1 - s2;
                const float outC = -outD;

                const float dryL = left[n];
                const float dryR = right[n];
                const float wetL = outA;
                const float wetR = outC;

                const float mix = mix_();
                left[n] = std::lerp(dryL, wetL, mix);
                right[n] = std::lerp(dryR, wetR, mix);
            }
        }

        void resetInner() noexcept override
        {
            for (auto& ap : allPasses_) {
                ap.clear();
            }
            for (auto& comb : combs_) {
                comb.clear();
            }
        }

    private:
        static constexpr std::size_t kAllPassCount = 3;
        static constexpr std::size_t kCombCount = 4;

        std::array<SchroederAllPass, 3> allPasses_;

        std::array<CombFilter, kCombCount> combs_;

        parameter::FloatParameterView mixParam_;
        FloatSmoother mix_;
    };

    Reverb::Reverb() : EffectBase("Reverb")
    {
        addProcessingStep(std::make_unique<SchroederReverb>(true));
    }
}