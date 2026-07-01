#pragma once

#include <vector>

#include "dsp/dsp.h"
#include "dsp/level_meter.h"
#include "dsp/effects/effect_chain.h"
#include "dsp/effects/pitch_shifter.h"
#include "dsp/effects/guitar_amp.h"
#include "dsp/effects/chorus.h"
#include "dsp/effects/equalizer.h"
#include "dsp/parameter/parameter_tree.h"
#include "dsp/parameter/parameter_view.h"

namespace ml::looper {
    class Mixer
    {
    public:
        explicit Mixer(const unsigned int nChannels);

        void prepare(float sampleRate);
        void process(float *const *data, const unsigned int nFrames);
        dsp::parameter::ParameterTree getParameterTree() const noexcept;
        std::pair<float*, float*> getChannelBuffers(const int index);
        std::pair<float, float> getLevel(const int index);

    private:
        void applyParams();

        struct Channel
        {
            dsp::FloatSmoother gain;
            dsp::FloatSmoother pan;
            dsp::LevelMeter meter;

            dsp::effects::EffectChain<
                dsp::effects::PitchShifter,
                dsp::effects::GuitarAmp,
                dsp::effects::Chorus,
                dsp::effects::Equalizer
            > fx{"FX"};

            dsp::parameter::FloatParameterView gainParam;
            dsp::parameter::FloatParameterView panParam;

            std::vector<float> bufferL;
            std::vector<float> bufferR;
        };

        std::vector<Channel> channels_;
        dsp::parameter::ParameterTree paramTree_;
    };
}