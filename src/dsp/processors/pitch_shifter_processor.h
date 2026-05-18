#pragma once

#include <cassert>

namespace dsp::processors {
    class PitchShifterProcessor {
    public:
        static constexpr unsigned int kNumChannels = 2;
        static constexpr unsigned int kMaxFrames = 4096;

        PitchShifterProcessor()
        {}

        void setPitchRatio(float pitchRatio)
        {
        }

        void prepare(float sampleRate)
        {
        }

        void reset()
        {
        }

        void process(float* const* data, unsigned int nFrames)
        {
            assert(nFrames <= kMaxFrames);
        }

    private:
    };
}