#include "looper_mixer.h"

#include <cassert>
#include <format>

#include "dsp/dsp.h"
#include "dsp/level_meter.h"
#include "dsp/effects/effect_chain.h"
#include "dsp/effects/pitch_shifter.h"
#include "dsp/effects/guitar_amp.h"
#include "dsp/effects/chorus.h"
#include "dsp/effects/equalizer.h"
#include "dsp/parameter/parameter_view.h"

#include "audio/audio_engine.h"

namespace ml::looper {
    struct Mixer::Channel
    {
        dsp::FloatSmoother gain;
        dsp::FloatSmoother pan;
        dsp::LevelMeter meter;

        dsp::effects::EffectChain<
            dsp::effects::PitchShifter,
            dsp::effects::GuitarAmp,
            dsp::effects::Chorus,
            dsp::effects::Equalizer
        > fx{"FX"};

        dsp::parameter::FloatParameterView gainParam;
        dsp::parameter::FloatParameterView panParam;

        std::vector<float> bufferL;
        std::vector<float> bufferR;

        ~Channel() = default;
    };

    Mixer::Mixer(const unsigned int nChannels)
        : paramTree_("LooperMixer")
    {
        using Param = dsp::parameter::Parameter;
        using ParamTree = dsp::parameter::ParameterTree;

        for (std::size_t i{}; i < nChannels; ++i) {
            auto& channel = channels_.emplace_back(std::make_unique<Channel>());
            const auto name = std::format("Track {}", i + 1);

            auto channelSubTree =  paramTree_.addSubTree({name, {
                ParamTree(Param::makeFloat("GainDb", 0.0f, dsp::Range{-60.0f, 12.0f})),
                ParamTree(Param::makeFloat("Pan", 0.0f, dsp::Range{-1.0f, 1.0f})),
                channel->fx.getParameterTree(),
            }});

            channel->gainParam.referTo(channelSubTree["GainDb"].asParameterUnsafe());
            channel->panParam.referTo(channelSubTree["Pan"].asParameterUnsafe());
        }
    }

    Mixer::~Mixer() = default;
    Mixer::Mixer(Mixer&&) noexcept = default;
    Mixer& Mixer::operator=(Mixer&&) noexcept = default;

    void Mixer::prepare(float sampleRate)
    {
        static constexpr float kSmoothingMs = 1.0f;
        const auto smoothFrames = kSmoothingMs * sampleRate * 0.001f;

        for (auto& channel : channels_) {
            channel->gain.setSmoothingFrames(smoothFrames);
            channel->pan.setSmoothingFrames(smoothFrames);

            channel->gain.init(dsp::dBtoLinear(channel->gainParam.get()));
            channel->pan.init(channel->panParam.get());

            channel->meter.prepare(sampleRate);
            channel->bufferL.assign(audio::kMaxFramesInBuffer, 0.0f);
            channel->bufferR.assign(audio::kMaxFramesInBuffer, 0.0f);

            channel->fx.prepare(sampleRate);
        }
    }

    void Mixer::process(float *const *data, const unsigned int nFrames)
    {
        applyParams();

        for (auto& channel : channels_) {
            float *const bufs[2] = {channel->bufferL.data(), channel->bufferR.data()};
            channel->fx.process(bufs, nFrames);

            for (auto i{0u}; i < nFrames; ++i) {
                auto [leftGain, rightGain] = dsp::equalPowerPanGains(channel->pan());
                const auto gain = channel->gain();
                leftGain *= gain;
                rightGain *= gain;

                const auto leftSample = channel->bufferL[i] * leftGain;;
                const auto rightSample = channel->bufferR[i] * rightGain;
                channel->meter(leftSample, rightSample);
                data[0][i] += leftSample;
                data[1][i] += rightSample;
            }
        }
    }

    dsp::parameter::ParameterTree Mixer::getParameterTree() const noexcept { return paramTree_; }

    std::pair<float*, float*> Mixer::getChannelBuffers(const int index)
    {
        assert((index >= 0) && (index < static_cast<int>(channels_.size())));
        auto &channel = channels_[index];
        return {channel->bufferL.data(), channel->bufferR.data()};
    }

    std::pair<float, float> Mixer::getLevel(const int index)
    {
        assert((index >= 0) && (index < static_cast<int>(channels_.size())));
        auto &channel = channels_[index];
        return channel->meter.getLevel();
    }

    void Mixer::applyParams()
    {
        for (auto& channel : channels_) {
            channel->gain.setTarget(dsp::dBtoLinear(channel->gainParam.get()));
            channel->pan.setTarget(channel->panParam.get());
        }
    }
}