#include "looper.h"

#include <algorithm>
#include <iostream>
#include <numbers>
#include <cmath>

#include "audio/audio_engine.h"
#include "looper_processor.h"
#include "looper_commands.h"

namespace looper {

    class LooperCallback : public audio::AudioCallback
    {
    public:
        void onProcess(const float *const *in, float *const *out, const unsigned int nFrames) override
        {
            const auto& engine = audio::AudioEngine::getInstance();
            const auto iChannels = engine.getNumInputChannels();
            const auto oChannels = engine.getNumOutputChannels();

            if (iChannels > 0 && iChannels == oChannels) {
                for (auto c{0u}; c < oChannels; ++c) {
                    for (auto i{0u}; i < nFrames; ++i) {
                        out[c][i] = in[c][i];
                    }
                }
            }

            looper.process(out, nFrames);

            /*const auto sr = static_cast<float>(engine.getSampleRate());
            constexpr auto twoPi = 2.0f * std::numbers::pi_v<float>;
            const float phaseIncr = twoPi * 440.0f / sr;
            static float osc{0};
            for (auto i{0u}; i < nFrames; ++i) {
                osc += phaseIncr;
                if (osc >= twoPi) osc -= twoPi;
                const float sine = std::sin(osc) * 0.03f;
                for (auto c{0u}; c < oChannels; ++c) {
                    out[c][i] = sine;
                }
            }*/
        }

        void onStart() override
        {
            //std::cout << "onStart()\n";
            looper.onStart();
        }

        void onStop() override
        {
            //std::cout << "onStop()\n";
            looper.onStop();
        }

        looper::LooperProcessor looper;
    };

    Looper::Looper() : cb_(std::make_shared<LooperCallback>())
    {
        auto& engine = audio::AudioEngine::getInstance();
        engine.setAudioCallback(cb_);
    }

    Looper::LooperState Looper::getLooperState() const noexcept
    {
        const auto nFrames = cb_->looper.getCurrentNumFrames();
        const auto loopPosition = cb_->looper.getCurrentPosition();
        const auto looperState = cb_->looper.getState();

        return LooperState {
            .nFrames = nFrames,
            .position = loopPosition,
            .state = looperState,
        };
    }

    void Looper::startRecording()
    {
        auto &looperMailbox = cb_->looper.getCommandMailbox();
        looperMailbox.tryPush(looper::LooperCommand::startRecording());
    }

    void Looper::stopRecording()
    {
        auto &looperMailbox = cb_->looper.getCommandMailbox();
        looperMailbox.tryPush(looper::LooperCommand::stopRecording());
    }

    void Looper::clear()
    {
        auto &looperMailbox = cb_->looper.getCommandMailbox();
        looperMailbox.tryPush(looper::LooperCommand::clear());
    }

}
