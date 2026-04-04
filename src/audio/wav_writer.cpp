#include "wav_writer.h"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#include <stdexcept>

namespace audio {
    WavWriter::WavWriter(const char* fileName, const unsigned int sampleRate, const unsigned int nChannels)
            : nChannels_(nChannels)
    {
        const drwav_data_format format = {
            .container = drwav_container_riff,
            .format = DR_WAVE_FORMAT_IEEE_FLOAT,
            .channels = nChannels,
            .sampleRate = sampleRate,
            .bitsPerSample = 32,
        };

        if (!drwav_init_file_write(&wav_, fileName, &format, nullptr)) {
            throw std::runtime_error("Failed to open wav file for writing");
        }
    }

    WavWriter::~WavWriter()
    {
        drwav_uninit(&wav_);
        tempBuffer_.fill(0);
    }

    void WavWriter::writeFrames(const float *const *data, const unsigned int nFrames)
    {
        unsigned int toWrite = nFrames;
        unsigned int offset = 0;

        while (toWrite > 0) {
            const auto toWriteNow = std::min(toWrite, static_cast<unsigned int>(tempBuffer_.size() / nChannels_));

            for (unsigned int channel = 0; channel < nChannels_; ++channel) {
                for (unsigned int i = 0; i < toWriteNow; ++i) {
                    tempBuffer_[i * nChannels_ + channel] = data[channel][offset+ i];
                }
            }

            drwav_write_pcm_frames(&wav_, toWriteNow, tempBuffer_.data());

            toWrite -= toWriteNow;
            offset += toWriteNow;
        }
    }
}