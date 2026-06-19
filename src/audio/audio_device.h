#pragma once

#include <string>
#include <vector>

namespace ml::audio {
    using DeviceIndex = int;
    inline constexpr DeviceIndex kNoDevice = -100;

    struct AudioDevice
    {
        DeviceIndex deviceIndex;
        std::string deviceName;
        std::string hostApiName;
        unsigned int maxInputChannels{2};
        unsigned int maxOutputChannels{2};
        std::vector<unsigned int> supportedSampleRates;

        void printInfo() const;
    };
}