#pragma once

#include <algorithm>
#include <cassert>
#include <vector>

#include "audio/audio_backend.h"
#include "audio/audio_engine.h"
#include "dsp/dsp.h"
#include "dsp/level_meter.h"

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
                channel.meter.prepare(static_cast<float>(audio::AudioEngine::getInstance().getSampleRate()));
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

        void process(float *const *data, const unsigned int nFrames)
        {
            float *leftData = data[0];
            float *rightData = data[1];

            for (auto& channel : channels) {
                auto [leftGain, rightGain] = dsp::equalPowerPanGains(channel.pan);
                leftGain *= channel.gain;
                rightGain *= channel.gain;

                for (auto i{0u}; i < nFrames; ++i) {
                    const auto leftSample = channel.bufferL[i] * leftGain;;
                    const auto rightSample = channel.bufferR[i] * rightGain;
                    channel.meter(leftSample, rightSample);
                    leftData[i] += leftSample;
                    rightData[i] += rightSample;
                }
            }
        }
        struct Channel
        {
            float gain;
            float pan;
            dsp::LevelMeter meter;
            std::vector<float> bufferL;
            std::vector<float> bufferR;
        };

        std::vector<Channel> channels;
    };
}
