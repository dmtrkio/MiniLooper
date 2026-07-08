#include "wav.h"

#include <iostream>
#include <stdexcept>

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

namespace ml::audio {
    std::expected<WavData, std::string> readWavFile(const std::filesystem::path& filePath) noexcept
    {
        try {
            drwav wav{};
            if (!drwav_init_file(&wav, filePath.string().c_str(), nullptr)) {
                return std::unexpected("Failed to open WAV file: " + filePath.string());
            }

            WavData result;
            result.sampleRate = static_cast<int>(wav.sampleRate);
            result.nChannels = static_cast<int>(wav.channels);
            result.frameCount = static_cast<int>(wav.totalPCMFrameCount);
            const auto totalFrames = wav.totalPCMFrameCount;

            if (result.nChannels == 0) {
                drwav_uninit(&wav);
                return std::unexpected("WAV file has no channels");
            }

            if (totalFrames == 0) {
                drwav_uninit(&wav);
                return result;
            }

            result.data.resize(result.nChannels);
            for (int ch = 0; ch < result.nChannels; ++ch) {
                result.data[ch].resize(static_cast<std::size_t>(totalFrames));
            }

            constexpr size_t chunkFrames = 4096;
            std::vector<float> tempBuffer(static_cast<size_t>(chunkFrames) * result.nChannels);

            drwav_uint64 framesReadTotal = 0;
            while (framesReadTotal < totalFrames) {
                const drwav_uint64 framesRemaining = totalFrames - framesReadTotal;
                const drwav_uint64 framesToRead = std::min(static_cast<drwav_uint64>(chunkFrames), framesRemaining);
                const drwav_uint64 framesRead = drwav_read_pcm_frames_f32(&wav, framesToRead, tempBuffer.data());

                if (framesRead == 0) break;

                for (drwav_uint64 i = 0; i < framesRead; ++i) {
                    for (int ch = 0; ch < result.nChannels; ++ch) {
                        result.data[ch][static_cast<std::size_t>(framesReadTotal + i)] = 
                            tempBuffer[static_cast<std::size_t>(i * result.nChannels + ch)];
                    }
                }

                framesReadTotal += framesRead;
            }

            drwav_uninit(&wav);

            if (framesReadTotal < totalFrames) {
                result.frameCount = static_cast<int>(framesReadTotal);
                for (int ch = 0; ch < result.nChannels; ++ch) {
                    result.data[ch].resize(static_cast<std::size_t>(framesReadTotal));
                }
            }

            return result;
        } catch (const std::bad_alloc&) {
            return std::unexpected("Memory allocation failed while reading WAV file");
        } catch (const std::exception& e) {
            return std::unexpected(std::string("Exception while reading WAV file: ") + e.what());
        } catch (...) {
            return std::unexpected("Unknown exception while reading WAV file");
        }
    }

    WavWriter::WavWriter(const std::filesystem::path& filePath, const unsigned int sampleRate, const unsigned int nChannels)
            : nChannels_(nChannels)
    {
        const drwav_data_format format = {
            .container = drwav_container_riff,
            .format = DR_WAVE_FORMAT_IEEE_FLOAT,
            .channels = nChannels,
            .sampleRate = sampleRate,
            .bitsPerSample = 32,
        };

        if (!drwav_init_file_write(&wav_, filePath.string().c_str(), &format, nullptr)) {
            throw std::runtime_error("drwav_init_file_write failed");
        }
    }

    WavWriter::~WavWriter()
    {
        if (drwav_uninit(&wav_) != DRWAV_SUCCESS) {
            std::cerr << "drwav_uninit failed" << std::endl;
        }
        tempBuffer_.fill(0);
    }

    void WavWriter::writeFrames(const float *const *data, const unsigned int nFrames)
    {
        drwav_uint64 toWrite = nFrames;
        drwav_uint64 offset = 0;

        while (toWrite > 0) {
            const auto toWriteNow = std::min(toWrite, static_cast<drwav_uint64>(tempBuffer_.size() / nChannels_));

            for (unsigned int channel = 0; channel < nChannels_; ++channel) {
                for (unsigned int i = 0; i < toWriteNow; ++i) {
                    tempBuffer_[i * nChannels_ + channel] = data[channel][offset+ i];
                }
            }

            const auto framesWritten = drwav_write_pcm_frames(&wav_, toWriteNow, tempBuffer_.data());
            toWrite -= framesWritten;
            offset += framesWritten;
        }
    }
}