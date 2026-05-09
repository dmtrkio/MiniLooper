#include <vector>
#include <array>

#include <signalsmith-stretch/signalsmith-stretch.h>

#include "dsp/effects/effect_base.h"
#include "dsp/parameter/parameter_tree.h"
#include "dsp/dsp.h"

namespace dsp::effects {
    class PitchShifter : public EffectBase
    {
    public:
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
        
        using Stretch = signalsmith::stretch::SignalsmithStretch<float>;
        Stretch shifter_;
        FloatSmoother semitones_;
        StereoFloatSmoother mix_;
        bool on_{false};

        const std::size_t kBufferSize = 8046;
        std::array<std::vector<float>, 2> buffers_; 
    };
}