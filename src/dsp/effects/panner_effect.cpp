#include "panner_effect.h"

namespace ml::dsp::effects {
    PannerEffect::PannerEffect() : EffectBase("Pan")
    {
        std::vector<ParamTree> params = {
            ParamTree{Param::makeFloat("Pan", 0.0f, dsp::Range{-1.0f, 1.0f})},
        };

        attachParameters(params);

        panParam_.referTo(params[0].asParameterUnsafe());
    }

    void PannerEffect::prepare(float sampleRate)
    {
        static constexpr float kSmoothingMs = 1.0f;
        const auto smoothFrames = kSmoothingMs * sampleRate * 0.001f;

        pan_.init(panParam_.get());
        pan_.setSmoothingFrames(smoothFrames);
    }

    void PannerEffect::processInner(float *const *data, const unsigned int nFrames) noexcept
    {
        pan_.setTarget(panParam_.get());

        for (auto i{0u}; i < nFrames; ++i) {
            auto [leftGain, rightGain] = dsp::equalPowerPanGains(pan_());
            data[0][i] = data[0][i] * leftGain;
            data[1][i] = data[1][i] * rightGain;
        }
    }
}