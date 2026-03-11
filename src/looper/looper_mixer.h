#pragma once

#include <algorithm>
#include <cassert>
#include <vector>

#include "audio/audio_backend.h"
#include "audio/audio_engine.h"
#include "dsp/dsp.h"
#include "dsp/level_meter.h"
#include "../dsp/parameter/audio_parameter.h"
#include "dsp/effects/equalizer.h"

namespace looper {
    struct MixerParams
    {
        using Param = dsp::parameter::Parameter;

        struct ChannelParams
        {
            Param gainDb;
            Param pan;
            dsp::parameter::ParameterTree *eqParamTree;
        };

        std::vector<ChannelParams> channels;

        explicit MixerParams(const unsigned int nChannels)
        {
            channels.reserve(nChannels);
            for (auto i{0u}; i < nChannels; ++i) {
                channels.emplace_back(ChannelParams{
                    .gainDb = Param::makeFloat("GainDb", 0.0f, dsp::Range{-60.0f, 12.0f}),
                    .pan = Param::makeFloat("Pan", 0.0f, dsp::Range{-1.0f, 1.0f}),
                    .eqParamTree = nullptr,
                });
            }
        }
    };

    struct Mixer
    {
        explicit Mixer(const unsigned int nChannels) : params(nChannels) {}

        void prepare()
        {
            const auto nChannels = params.channels.size();
            const auto sampleRate = static_cast<float>(audio::AudioEngine::getInstance().getSampleRate());

            static constexpr float kSmoothingMs = 1.0f;
            const auto smoothFrames = kSmoothingMs * sampleRate * 0.001f;

            channels.assign(nChannels, {});
            for (auto i{0u}; i < nChannels; ++i) {
                auto &channel = channels[i];
                const auto &chParams = params.channels[i];
                channel.gain.init(dsp::dBtoLinear(chParams.gainDb.get<float>()));
                channel.pan.init(chParams.pan.get<float>());
                channel.gain.setSmoothingFrames(smoothFrames);
                channel.pan.setSmoothingFrames(smoothFrames);

                channel.meter.prepare(sampleRate);
                channel.bufferL.assign(audio::kMaxFramesInBuffer, 0.0f);
                channel.bufferR.assign(audio::kMaxFramesInBuffer, 0.0f);

                channel.eq.prepare(sampleRate);
                params.channels[i].eqParamTree = &channel.eq.getParameterTree();
            }
        }

        void applyParams()
        {
            const auto nChannels = params.channels.size();
            assert(channels.size() == nChannels);

            for (auto i{0u}; i < nChannels; ++i) {
                auto &channel = channels[i];
                const auto &chParams = params.channels[i];
                channel.gain.setTarget(dsp::dBtoLinear(chParams.gainDb.get<float>()));
                channel.pan.setTarget(chParams.pan.get<float>());
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
            applyParams();

            for (auto& channel : channels) {
                float *bufs[2] = { channel.bufferL.data(), channel.bufferR.data() };
                channel.eq(bufs, nFrames);

                auto [leftGain, rightGain] = dsp::equalPowerPanGains(channel.pan());
                const auto gain = channel.gain();
                leftGain *= gain;
                rightGain *= gain;

                for (auto i{0u}; i < nFrames; ++i) {
                    const auto leftSample = channel.bufferL[i] * leftGain;;
                    const auto rightSample = channel.bufferR[i] * rightGain;
                    channel.meter(leftSample, rightSample);
                    data[0][i] += leftSample;
                    data[1][i] += rightSample;
                }
            }
        }

        struct Channel
        {
            dsp::FloatSmoother gain;
            dsp::FloatSmoother pan;
            dsp::LevelMeter meter;
            dsp::effects::Equalizer eq;

            std::vector<float> bufferL;
            std::vector<float> bufferR;
        };

        std::vector<Channel> channels;

        MixerParams params;
    };
}
