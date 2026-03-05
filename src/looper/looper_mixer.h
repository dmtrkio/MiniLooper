#pragma once

#include <vector>

#include "audio/audio_engine.h"
#include "dsp/dsp.h"

namespace looper {
    struct MixerChannel
    {
        std::vector<float> bufferL;
        std::vector<float> bufferR;
    };

    class Mixer
    {
    public:
        void onStart()
        {

        }

    };
}