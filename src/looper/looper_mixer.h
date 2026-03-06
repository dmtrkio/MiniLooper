#pragma once

#include <algorithm>
#include <cassert>
#include <vector>

#include "audio/audio_engine.h"
#include "dsp/dsp.h"

namespace looper {
    class Mixer
    {
    public:
        void prepare(unsigned int nMixerChannels)
        {
            channels.assign(nMixerChannels, {});
            for (auto& channel : channels) {
                channel.gain = 1.0f;
                channel.pan = 0.0f;
                channel.bufferL.assign(audio::kMaxFramesInBuffer, 0.0f);
                channel.bufferR.assign(audio::kMaxFramesInBuffer, 0.0f);
            }
        }

        std::pair<float*, float*> getChannelBuffers(const int index)
        {
            assert((index >= 0) && (index < channels.size()));
            auto &channel = channels[index];
            return {channel.bufferL.data(), channel.bufferR.data()};
        }

        void process(float *const *data, const unsigned int nFrames) const
        {
            float *leftData = data[0];
            float *rightData = data[1];

            for (const auto& channel : channels) {
                auto [leftGain, rightGain] = dsp::equalPowerPanGains(channel.pan);
                leftGain *= channel.gain;
                rightGain *= channel.gain;

                for (auto i{0u}; i < nFrames; ++i) {
                    leftData[i] += channel.bufferL[i] * leftGain;
                }

                for (auto i{0u}; i < nFrames; ++i) {
                    rightData[i] += channel.bufferR[i] * rightGain;
                }
            }
        }

    private:
        struct MixerChannel
        {
            float gain;
            float pan;
            std::vector<float> bufferL;
            std::vector<float> bufferR;
        };

        std::vector<MixerChannel> channels;
    };
}