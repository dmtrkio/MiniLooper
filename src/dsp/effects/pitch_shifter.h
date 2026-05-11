#include <vector>
#include <array>
#include <memory>

#include "dsp/effects/effect_base.h"
#include "dsp/parameter/parameter_tree.h"
#include "dsp/dsp.h"
#include "dsp/delay_line.h"

namespace dsp::effects {
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
        
        struct PitcherState;
        std::unique_ptr<PitcherState> pitcher_;

        FloatSmoother semitones_;
        StereoFloatSmoother mix_;
        bool on_{false};

        static constexpr std::size_t kBufferSize = 8046;
        std::array<std::vector<float>, 2> buffers_; 
        std::array<FractionalDelayLine, 2> delay_;
    };
}