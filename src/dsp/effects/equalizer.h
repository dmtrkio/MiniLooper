#pragma once

#include <vector>

#include "dsp/filter/biquad_filter.h"
#include "dsp/audio_parameter.h"
#include "dsp/dsp.h"

namespace dsp::effects {
    class Equalizer
    {
    public:
        void prepare(const float sampleRate)
        {

        }

    private:
        using Filter = filter::BiquadFilter<double>;
        struct EqBand
        {
            filter::FilterType type;
            Filter::Coefficients coefficients;
            Filter leftState;
            Filter rightState;
        };

        EqBand lowPass_{};
        EqBand lowShelf{};
        EqBand peak1_{};
        EqBand peak2_{};
        EqBand highShelf{};
        EqBand highPass_{};
    };
}