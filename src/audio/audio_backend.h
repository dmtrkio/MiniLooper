#pragma once

#include <functional>
#include <vector>
#include <string>
#include <iostream>

namespace audio {
    struct AudioDevice
    {
        int deviceIndex;
        std::string deviceName;
        std::string hostApiName;
        unsigned int maxInputChannels{2};
        unsigned int maxOutputChannels{2};
        std::vector<unsigned int> supportedSampleRates;

        void printInfo() const
        {
            std::cout << std::endl;
            std::cout << "Device index: " << deviceIndex << std::endl;
            std::cout << "  Device name: " << deviceName << std::endl;
            std::cout << "  Host Api: " << hostApiName << std::endl;
            std::cout << "  Number of input channels: " << maxInputChannels << std::endl;
            std::cout << "  Number of output channels: " << maxOutputChannels << std::endl;
            std::cout << "  Supported sample rates: [";
            for (const auto sr : supportedSampleRates) {
                std::cout << sr << " ";
            }
            std::cout << "]" << std::endl;
        }
    };

    class AudioBackend
    {
    public:
        using Callback = std::function<bool(const float *in, float *out, unsigned int nFrames)>;

        struct StreamParams
        {
            unsigned int sampleRate{44100};
            unsigned int bufferSize{512};
            unsigned int numInputChannels{2};
            unsigned int numOutputChannels{2};
        };

        explicit AudioBackend(Callback audioCallback) : audioCallback_(std::move(audioCallback)) {}

        virtual ~AudioBackend() = default;

        AudioBackend(const AudioBackend&) = delete;
        AudioBackend& operator=(const AudioBackend&) = delete;

        AudioBackend(AudioBackend&&) noexcept = default;
        AudioBackend& operator=(AudioBackend&&) noexcept = default;

        [[nodiscard]] virtual std::vector<AudioDevice> getAvailableDevices() = 0;

        virtual bool startStream(int inputDeviceIndex, int outputDeviceIndex, StreamParams &params) = 0;
        virtual bool stopStream() = 0;
        [[nodiscard]] virtual bool isStreamRunning() const = 0;

    protected:
        Callback audioCallback_;
    };

}
