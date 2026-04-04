#pragma once

#include <array>

#include "dr_wav.h"

namespace audio {
    class WavWriter
    {
    public:
        WavWriter(const char* fileName, unsigned int sampleRate, unsigned int nChannels);
        ~WavWriter();

        WavWriter(const WavWriter&) = delete;
        WavWriter(WavWriter&&) = delete;
        WavWriter& operator=(const WavWriter&) = delete;
        WavWriter& operator=(WavWriter&&) = delete;

        void writeFrames(const float *const *data, unsigned int nFrames);

    private:
        unsigned int nChannels_;
        drwav wav_{};
        std::array<float, 4096> tempBuffer_{};
    };
}