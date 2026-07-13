#include "pitch_shifter.h"

namespace ml::dsp::effects {
    PitchShifter::PitchShifter() : EffectBase("PitchShifter")
    {
        std::vector<ParamTree> params = {
            ParamTree{Param::makeInteger("Semitones", 0, {-12, 12})},
            ParamTree{Param::makeFloat("Mix", 1.0f, {0.0f, 1.0f})},
        };

        attachParameters(params);

        semitonesParam_.referTo(params[0].asParameterUnsafe());
        mixParam_.referTo(params[1].asParameterUnsafe());
    }

    void PitchShifter::prepareInner(float sampleRate)
    {
        static constexpr float kSmoothingMs = 1.0f;
        const auto smoothFrames = kSmoothingMs * sampleRate * 0.001f;

        semitones_.setSmoothingFrames(smoothFrames);
        mix_.setSmoothingFrames(smoothFrames);

        semitones_.init(static_cast<float>(semitonesParam_.get()));
        mix_.init(mixParam_.get());

        shifter_.prepare(sampleRate);
    }

    void PitchShifter::processInner(float *const *data, unsigned int nFrames) noexcept
    {
        semitones_.setTarget(static_cast<float>(semitonesParam_.get()));
        mix_.setTarget(mixParam_.get());

        for (std::size_t i{0u}; i < nFrames; ++i) {
            shifter_.setSemitones(semitones_());
            const auto sum = (data[0][i] + data[1][i]) * 0.5f;
            const auto output = shifter_.process(sum);
            data[0][i] = std::lerp(sum, output, mix_());
            data[1][i] = data[0][i];
        }
    }

    void PitchShifter::resetInner() noexcept
    {
        shifter_.clearState();
    }
}