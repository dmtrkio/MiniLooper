#include "reverb.h"

#include "dsp/dsp.h"
#include "dsp/filter/reverb_design.h"
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
                ParamTree{Param::makeFloat("Decay", 0.5f, dsp::Range{0.0f, 1.0f})},
                ParamTree{Param::makeFloat("Damp", 0.5f, dsp::Range{0.0f, 1.0f})},
            };

            attachParameters(params);

            mixParam_.referTo(params[0].asParameterUnsafe());
            decayParam_.referTo(params[1].asParameterUnsafe());
            dampParam_.referTo(params[2].asParameterUnsafe());
        }

    protected:
        void prepareInner(float sampleRate) override
        {
            static constexpr float kSmoothingMs = 1.0f;
            const auto smoothFrames = kSmoothingMs * sampleRate * 0.001f;

            decay_.init(decayParam_.get());
            decay_.setSmoothingFrames(smoothFrames);

            mix_.init(dsp::dBtoLinear(mixParam_.get()));
            mix_.setSmoothingFrames(smoothFrames);

            const auto convertSr = [sampleRate](float x) {
                return x * (sampleRate / 25000.0f);
            };

            for (std::size_t i = 0; i < allPasses_.size(); ++i) {
                const float delay = convertSr(kAllPassDelays[i]);
                allPasses_[i].prepare(delay);
                allPasses_[i].setDelay(delay);
                allPasses_[i].setGain(kAllPassGain);
            }

            for (std::size_t i = 0; i < combs_.size(); ++i) {
                const float delay = convertSr(kCombDelays[i]);
                combs_[i].prepare(delay);
                combs_[i].setDelay(delay);
                combs_[i].setFeedback(kCombFeedback[i]);
                combs_[i].setDamp(dampParam_.get());
            }
        }

        void processInner(float *const *data, unsigned int nFrames) noexcept override
        {
            mix_.setTarget(mixParam_.get());
            decay_.setTarget(decayParam_.get());
            damp_.setTarget(dampParam_.get());

            float* left = data[0];
            float* right = data[1];

            for (unsigned int n = 0; n < nFrames; ++n) {
                setDecay(decay_());
                setDamp(damp_());

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
        void setDecay(float decay) noexcept
        {
            const float decayScaler = std::lerp(0.4f, 1.0f, decay);
            for (std::size_t i = 0; i < combs_.size(); ++i) {
                combs_[i].setFeedback(decayScaler * kCombFeedback[i]);
            }
        }

        void setDamp(float damp) noexcept
        {
            const float dampMapped = std::lerp(0.05f, 0.85f, damp);
            for (auto& comb : combs_) {
                comb.setDamp(dampMapped);
            }
        }

        static constexpr std::size_t kAllPassCount = 3;
        static constexpr std::size_t kCombCount = 4;

        static constexpr std::array<float, kAllPassCount> kAllPassDelays = { 347.0f, 113.0f, 37.0f };
        static constexpr float kAllPassGain = 0.7f;

        static constexpr std::array<float, kCombCount> kCombDelays = { 1687.0f, 1601.0f, 2053.0f, 2251.0f };
        static constexpr std::array<float, kCombCount> kCombFeedback = {0.773f, 0.802f, 0.753f, 0.733f };

        std::array<filter::SchroederAllPass, kAllPassCount> allPasses_;

        std::array<filter::LowpassFeedbackCombFilter, kCombCount> combs_;

        parameter::FloatParameterView mixParam_;
        FloatSmoother mix_;

        parameter::FloatParameterView decayParam_;
        FloatSmoother decay_;

        parameter::FloatParameterView dampParam_;
        FloatSmoother damp_;
    };

    Reverb::Reverb() : EffectBase("Reverb")
    {
        addProcessingStep(std::make_unique<SchroederReverb>(true));
    }
}