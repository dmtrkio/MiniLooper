#include <vector>

#include "dsp/effects/effect_base.h"
#include "dsp/parameter/parameter_tree.h"
#include "dsp/parameter/parameter_view.h"
#include "dsp/simple_pitch_shifter.h"

namespace ml::dsp::effects {
    class PitchShifter : public EffectBase
    {
    public:
        PitchShifter();
        ~PitchShifter();

        PitchShifter(const PitchShifter&) = delete;
        PitchShifter& operator=(const PitchShifter&) = delete;

        PitchShifter(PitchShifter&&) noexcept;
        PitchShifter& operator=(PitchShifter&&) noexcept;

        void prepare(float sampleRate) override;
        void process(float *const *data, unsigned int nFrames) override;
        parameter::ParameterTree getParameterTree() const override;

    private:
        void setOn();

        using Param = parameter::Parameter;
        using ParamTree = parameter::ParameterTree;
        ParamTree paramTree_ {"PitchShifter", {
            Param::makeBoolean("On", false),
            Param::makeInteger("Semitones", 0, {-12, 12}),
            Param::makeFloat("Mix", 0.0f, {0.0f, 1.0f}),
        }};

        parameter::BooleanParameterView onParam_;
        parameter::IntegerParameterView semitonesParam_;
        parameter::FloatParameterView mixParam_;

        SimplePitchShifter shifter_;

        FloatSmoother semitones_;
        FloatSmoother mix_;
        bool on_{false};
    };
}