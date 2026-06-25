#pragma once

#include "dsp.h"
#include "rt_sanitizer.h"
#include "oscillator.h"
#include "filter/ballistics_filter.h"

namespace ml::dsp {
    class ClickGenerator
    {
    public:
        void  prepare(float sampleRate);
        float process(bool click) RT_SAN;

    private:
        float sampleRate_{48000.0f};
        filter::BallisticsFilter pitchEnv_;
        filter::BallisticsFilter ampEnv_;
        Oscillator osc_;
    };
}