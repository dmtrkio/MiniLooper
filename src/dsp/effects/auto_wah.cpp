#include "auto_wah.h"

#include "dsp/filter/biquad_coefficients.h"
#include "dsp/dsp.h"

namespace ml::dsp::effects {
    static constexpr float kMinFrequency = 260.0f;
    static constexpr float kMaxFrequency = 3200.0f;
    static constexpr float kAttackMs = 2.0f;
    static constexpr float kReleaseMs = 230.0f;
    static constexpr float kMinSensitivity = 1.0f;
    static constexpr float kMaxSensetivity = 5.0f;

    AutoWah::AutoWah()
        : EffectBase("AutoWah")
    {
        std::vector<ParamTree> params = {
            ParamTree{Param::makeFloat("Sensitivity", 0.5f, {0.0f, 1.0f})},
            ParamTree{Param::makeFloat("Drive", 0.0f, {0.0f, 20.0f})},
            ParamTree{Param::makeFloat("Q", 5.0f, {1.0f, 10.0f})},
        };

        attachParameters(params);

        sensetivityParam_.referTo(params[0].asParameterUnsafe());
        driveParam_.referTo(params[1].asParameterUnsafe());
        qParam_.referTo(params[2].asParameterUnsafe());
    }

    void AutoWah::prepareInner(float sampleRate)
    {
        sampleRate_ = sampleRate;

        filterEnvelope_.prepare(sampleRate, kAttackMs, kReleaseMs);

        static constexpr float kSmoothingMs = 1.0f;
        const auto smoothFrames = kSmoothingMs * sampleRate * 0.001f;

        sensetivity_.setSmoothingFrames(smoothFrames);
        drive_.setSmoothingFrames(smoothFrames);
        q_.setSmoothingFrames(smoothFrames);

        sensetivity_.init(sensetivityParam_.get());
        drive_.init(dBtoLinear(driveParam_.get()));
        q_.init(qParam_.get());
    }

    void AutoWah::processInner(float *const *data, unsigned int nFrames) noexcept
    {
        sensetivity_.setTarget(sensetivityParam_.get());
        drive_.setTarget(dBtoLinear(driveParam_.get()));
        q_.setTarget(qParam_.get());

        float* left = data[0];
        float* right = data[1];

        for (auto i{0u}; i < nFrames; ++i) {
            const float sensitivity = kMinSensitivity + sensetivity_() * (kMaxSensetivity - kMinSensitivity);
            const float q = drive_();
            const float drive = drive_();
            const float minF = kMinFrequency;
            const float maxF = kMaxFrequency;

            const float trigger = [&]() {
                const float l = std::fabs(left[i]);
                const float r = std::fabs(right[i]);
                return std::max(l, r);
            }();

            const float env = filterEnvelope_.process(trigger * sensitivity);
            const float frequency = minF + env * (maxF - minF);
            const float nf = filter::normalizeFrequency(sampleRate_, frequency);

            filter_.process(left[i], right[i], q, nf);

            left[i] = std::tanh(left[i] * drive);
            right[i] = std::tanh(right[i] * drive);
        }
    }
}