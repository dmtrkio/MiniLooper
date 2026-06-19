#include "audio_device.h"

#include <iostream>

namespace audio {
    void AudioDevice::printInfo() const
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
}