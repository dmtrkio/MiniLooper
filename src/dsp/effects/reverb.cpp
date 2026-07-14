#include "reverb.h"

#include "dsp/delay_line.h"

namespace ml::dsp::effects {
    class SchroederReverb : public EffectBase
    {
    public:
        explicit SchroederReverb(bool enabled = false)
            : EffectBase("Schroeder Reverb", enabled)
        {}

    protected:
        void prepareInner(float sampleRate) override
        {
            sampleRate_ = sampleRate;

            // Classic Schroeder delay times in milliseconds
            constexpr std::array<float, 4> combDelaysMs = {29.7f, 37.1f, 41.1f, 43.7f};
            constexpr std::array<float, 4> combFeedback = {0.805f, 0.827f, 0.783f, 0.764f};

            constexpr std::array<float, 2> allPassDelaysMs = {5.0f, 1.7f};
            constexpr float allPassGain = 0.7f;

            // Prepare comb filters
            for (size_t i = 0; i < combs_.size(); ++i) {
                float delaySamples = combDelaysMs[i] * sampleRate / 1000.0f;
                combs_[i].prepare(delaySamples + 1.0f); // +1 for safety
                combs_[i].setDelay(delaySamples);
                combFeedback_[i] = combFeedback[i];
            }

            // Prepare all-pass filters
            for (size_t i = 0; i < allPasses_.size(); ++i) {
                float delaySamples = allPassDelaysMs[i] * sampleRate / 1000.0f;
                allPasses_[i].prepare(delaySamples + 1.0f);
                allPasses_[i].setDelay(delaySamples);
            }

            allPassGain_ = allPassGain;
        }

        void processInner(float *const *data, unsigned int nFrames) noexcept override
        {
            float* left = data[0];
            float* right = data[1];

            for (unsigned int n = 0; n < nFrames; ++n) {
                float monoIn = (left[n] + right[n]) * 0.5f;

                float combSum = 0.0f;
                for (size_t i = 0; i < combs_.size(); ++i) {
                    combSum += combs_[i].processWithFeedback(monoIn, combFeedback_[i]);
                }
                combSum *= 0.25f; // Normalize

                float wet = combSum;
                for (auto& ap : allPasses_) {
                    wet = ap.processAllPass(wet, allPassGain_);
                }

                const float mix = 0.5f;
                left[n] = std::lerp(left[n], wet, mix);
                right[n] = std::lerp(right[n], wet, mix);
            }
        }

        void resetInner() noexcept override
        {
            for (auto& comb : combs_) {
                comb.clear();
            }
            for (auto& ap : allPasses_) {
                ap.clear();
            }
        }

    private:
        float sampleRate_ = 44100.0f;

        std::array<FractionalDelayLine, 4> combs_;
        std::array<float, 4> combFeedback_{};

        std::array<FractionalDelayLine, 2> allPasses_;
        float allPassGain_ = 0.7f;
    };

    Reverb::Reverb() : EffectBase("Reverb")
    {
        addProcessingStep(std::make_unique<SchroederReverb>(true));
    }
}