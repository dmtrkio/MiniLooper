#include "pitch_shifter.h"

namespace ml::dsp::effects {
    PitchShifter::PitchShifter()
    {
        onParam_.referTo(paramTree_["On"].asParameterUnsafe());
        semitonesParam_.referTo(paramTree_["Semitones"].asParameterUnsafe());
        mixParam_.referTo(paramTree_["Mix"].asParameterUnsafe());
    }

    PitchShifter::~PitchShifter() = default;

    PitchShifter::PitchShifter(PitchShifter&&) noexcept = default;
    PitchShifter& PitchShifter::operator=(PitchShifter&&) noexcept = default;

    void PitchShifter::prepare(float sampleRate)
    {
        static constexpr float kSmoothingMs = 1.0f;
        const auto smoothFrames = kSmoothingMs * sampleRate * 0.001f;

        semitones_.setSmoothingFrames(smoothFrames);
        mix_.setSmoothingFrames(smoothFrames);

        semitones_.init(static_cast<float>(semitonesParam_.get()));
        mix_.init(mixParam_.get());

        shifter_.prepare(sampleRate);

        setOn();
    }

    void PitchShifter::process(float *const *data, unsigned int nFrames)
    {
        semitones_.setTarget(static_cast<float>(semitonesParam_.get()));
        mix_.setTarget(mixParam_.get());

        setOn();
        if (!on_) return;

        for (std::size_t i{0u}; i < nFrames; ++i) {
            shifter_.setSemitones(semitones_());
            const auto sum = (data[0][i] + data[1][i]) * 0.5f;
            const auto output = shifter_.process(sum);
            data[0][i] = std::lerp(sum, output, mix_());
            data[1][i] = data[0][i];
        }
    }

    parameter::ParameterTree PitchShifter::getParameterTree() const
    {
        return paramTree_;
    }

    void PitchShifter::setOn()
    {
        const auto on = onParam_.get();
        if (!on && on_) {
            shifter_.clearState();
        }
        on_ = on;
    }
}