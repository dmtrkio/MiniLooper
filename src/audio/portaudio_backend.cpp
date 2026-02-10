#pragma once

#include <iostream>

#include <portaudio.h>

#include "audio_engine.h"

namespace audio {
    class PortaudioBackend : AudioBackend
    {
    public:
        bool initialize(Callback cb) override
        {
            if (const auto err = Pa_Initialize(); err != paNoError) {
                Pa_Terminate();
                std::cout << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
                return false;
            }

            return true;
        }

        ~PortaudioBackend() override
        {

        }

        bool startStream(StreamParams &params) override
        {
            return false;
        }

        bool stopStream() override
        {
            return false;
        }

        [[nodiscard]] bool isStreamRunning() const override
        {
            return false;
        }

    private:
        Callback callback_;
    };
} // audio