#pragma once

#include <iostream>
#include <string>
#include <vector>

namespace audio {
    using DeviceIndex = int;
    static constexpr DeviceIndex kNoDevice = -100;

    struct AudioDevice
    {
        DeviceIndex deviceIndex;
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
}