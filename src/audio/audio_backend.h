#pragma once

#include <functional>
#include <vector>
#include <expected>

#include "audio_device.h"

namespace ml::audio {
    class AudioBackend
    {
    public:
        using Callback = std::function<bool(const float *in, float *out, int nFrames)>;

        struct StreamParams
        {
            int sampleRate{44100};
            int bufferSize{512};
            int numInputChannels{2};
            int numOutputChannels{2};
        };

        explicit AudioBackend(Callback audioCallback) : audioCallback_(std::move(audioCallback)) {}

        virtual ~AudioBackend() = default;

        AudioBackend(const AudioBackend&) = delete;
        AudioBackend& operator=(const AudioBackend&) = delete;

        AudioBackend(AudioBackend&&) noexcept = default;
        AudioBackend& operator=(AudioBackend&&) noexcept = default;

        [[nodiscard]] virtual std::vector<AudioDevice> getAvailableDevices() = 0;

        [[nodiscard]] virtual std::expected<void, std::string> startStream(
            DeviceIndex &inputDeviceIndex,
            DeviceIndex &outputDeviceIndex,
            StreamParams &params
        ) = 0;

        [[nodiscard]] virtual std::expected<void, std::string> stopStream() = 0;

        [[nodiscard]] virtual bool isStreamRunning() const = 0;

    protected:
        Callback audioCallback_;
    };
}
