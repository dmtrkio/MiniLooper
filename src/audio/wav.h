#pragma once

#include <array>
#include <vector>
#include <filesystem>
#include <expected>

#include "dr_wav.h"

namespace ml::audio {
    struct WavData
    {
        int sampleRate;
        int nChannels;
        int frameCount;
        std::vector<std::vector<float>> data;
    };

    std::expected<WavData, std::string> readWavFile(const std::filesystem::path& filePath) noexcept;

    class WavWriter
    {
    public:
        WavWriter(const std::filesystem::path& filePath, unsigned int sampleRate, unsigned int nChannels);
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