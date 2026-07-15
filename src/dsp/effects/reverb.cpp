#include "reverb.h"

#include "dsp/delay_line.h"
#include "dsp/dsp.h"
#include "dsp/parameter/parameter_view.h"

namespace ml::dsp::effects {
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
                allPasses_[i].setDelay(convertSr(allPassDelays[i]));
            }
            allPassGain_ = allPassGain;

            for (std::size_t i = 0; i < combs_.size(); ++i) {
                combs_[i].setDelay(convertSr(combDelays[i]));
                combFeedback_[i] = combFeedback[i];
            }
        }

        void processInner(float *const *data, unsigned int nFrames) noexcept override
        {
            mix_.setTarget(mixParam_.get());

            float* left = data[0];
            float* right = data[1];

            for (unsigned int n = 0; n < nFrames; ++n) {
                const float monoIn = (left[n] + right[n]) * 0.5f;

                float allpassOutput = monoIn;
                for (auto& ap : allPasses_) {
                    allpassOutput = allpass(ap, allpassOutput, allPassGain_);
                }

                std::array<float, kCombCount> x;
                for (std::size_t i = 0; i < combs_.size(); ++i) {
                    x[i] = combs_[i].processWithFeedback(allpassOutput, combFeedback_[i]);
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
                const float wetL = outA + dryL * 0.25f;
                const float wetR = outC + dryR * 0.25f;

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
        static float allpass(FractionalDelayLine& delayLine, const float input, const float gain) noexcept
        {
            const float delayed = delayLine.read();
            const float w = input + gain * delayed;
            delayLine.write(w);
            return delayed - gain * w;
        }

        static constexpr std::size_t kAllPassCount = 3;
        static constexpr std::size_t kCombCount = 4;

        std::array<FractionalDelayLine, 3> allPasses_;
        float allPassGain_ = 0.7f;

        std::array<FractionalDelayLine, kCombCount> combs_;
        std::array<float, kCombCount> combFeedback_{};

        parameter::FloatParameterView mixParam_;
        FloatSmoother mix_;
    };

    Reverb::Reverb() : EffectBase("Reverb")
    {
        addProcessingStep(std::make_unique<SchroederReverb>(true));
    }
}