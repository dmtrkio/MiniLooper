#pragma once

#include <portaudio.h>

#include "audio_backend.h"

namespace audio {
    class PortAudioBackend final : public AudioBackend
    {
    public:
        explicit PortAudioBackend(Callback audioCallback);
        ~PortAudioBackend() override;

        std::vector<AudioDevice> getAvailableDevices() override;
        bool startStream(DeviceIndex &inputDeviceIndex, DeviceIndex &outputDeviceIndex, StreamParams &params) override;
        bool stopStream() override;
        [[nodiscard]] bool isStreamRunning() const override;

    private:
        static bool validateStreamParameters(int inputDeviceIndex, int outputDeviceIndex, StreamParams &params);
        void scanDevices();

        static int paCallback(
            const void *input,
            void *output,
            unsigned long frameCount,
            const PaStreamCallbackTimeInfo* timeInfo,
            PaStreamCallbackFlags statusFlags,
            void *userData
        );

        PaStream* stream_{nullptr};

        std::vector<AudioDevice> devices_;
    };

} // audio
