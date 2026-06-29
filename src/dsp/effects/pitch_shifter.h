#pragma once

#include "dsp/effects/effect_base.h"
#include "dsp/parameter/parameter_view.h"
#include "dsp/simple_pitch_shifter.h"

namespace ml::dsp::effects {
    class PitchShifter final : public EffectBase
    {
    public:
        PitchShifter();
        void prepare(float sampleRate) override;
        void reset() noexcept override;

    protected:
        void processInner(float *const *data, unsigned int nFrames) noexcept override;

    private:
        parameter::IntegerParameterView semitonesParam_;
        parameter::FloatParameterView mixParam_;

        SimplePitchShifter shifter_;

        FloatSmoother semitones_;
        FloatSmoother mix_;
    };
}