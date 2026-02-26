#include "looper.h"

#include <iostream>
#include <algorithm>
#include <numbers>
#include <cmath>

#include "audio/audio_engine.h"
#include "looper_processor.h"
#include "looper_commands.h"
#include "midi/foot_switch.h"
#include "timer.h"

namespace looper {

    class Looper::LooperCallback : public audio::AudioCallback
    {
    public:
        void onProcess(const float *const *in, float *const *out, const unsigned int nFrames) override
        {
            const auto& engine = audio::AudioEngine::getInstance();
            const auto iChannels = engine.getNumInputChannels();
            const auto oChannels = engine.getNumOutputChannels();

            for (auto i{0u}; i < nFrames; ++i) {
                float inputSample = 0.0f;
                for (auto c{0u}; c < iChannels; ++c) {
                    inputSample += in[c][i];
                }
                for (auto c{0u}; c < oChannels; ++c) {
                    out[c][i] = inputSample;
                }
            }

            for (auto i{0u}; i < nFrames; ++i)
                pressTimer.tick();

            drainMidiQueue();

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

            pressTimer.setOnTimeout([&] {
                const int trackIndex = 0;
                if (looper.getState(trackIndex) != looper::State::RECORDING) {
                    looper.startRecording(trackIndex);
                } else {
                    looper.stopRecording(trackIndex);
                }
            });
            pressTimer.setOneShot(true);
            const auto sampleRate = static_cast<float>(audio::AudioEngine::getInstance().getSampleRate());
            pressTimer.setTimeoutSecs(sampleRate, 0.15f);
            pressTimer.stop();
        }

        void onStop() override
        {
            //std::cout << "onStop()\n";

            looper.onStop();

            pressTimer.stop();
        }

        void drainMidiQueue()
        {
            midiQueue.consumeAll([&](const midi::MidiMessage& msg) {
                if (footSwitch.update(msg)) {
                    if (pressTimer.isRunning()) {
                        pressTimer.stop();
                        looper.clearAll();
                    } else {
                        pressTimer.start();
                    }
                }
            });
        }

        looper::LooperProcessor looper;
        midi::MidiQueue midiQueue{64};
        midi::FootSwitch footSwitch;
        timer::AudioRateTimer pressTimer;
    };

    Looper::Looper() : cb_(std::make_shared<LooperCallback>())
    {
        auto& engine = audio::AudioEngine::getInstance();
        engine.setAudioCallback(cb_);
    }

    int Looper::getNumLooperTracks() const noexcept
    {
        return cb_->looper.getNumLooperTracks();
    }

    void Looper::updateSnapshot() noexcept
    {
        const auto reader = cb_->looper.getSharedData().state.read();
        if (reader.isFresh()) {
            snapshot_ = reader.data();
        }
    }

    const TrackStateSnapshot& Looper::getTrackState(int trackIndex) const noexcept
    {
        assert(trackIndex >= 0 && trackIndex < getNumLooperTracks());
        return snapshot_.tracks[trackIndex];
    }

    void Looper::startRecording(int trackIndex)
    {
        auto &looperMailbox = getCommandMailbox();
        looperMailbox.tryPush(looper::LooperCommand::startRecording(trackIndex));
    }

    void Looper::stopRecording(int trackIndex)
    {
        auto &looperMailbox = getCommandMailbox();
        looperMailbox.tryPush(looper::LooperCommand::stopRecording(trackIndex));
    }

    void Looper::clear(int trackIndex)
    {
        auto &looperMailbox = getCommandMailbox();
        looperMailbox.tryPush(looper::LooperCommand::clear(trackIndex));
    }

    void Looper::pause(int trackIndex)
    {
        auto &looperMailbox = getCommandMailbox();
        looperMailbox.tryPush(looper::LooperCommand::pause(trackIndex));
    }

    void Looper::resume(int trackIndex)
    {
        auto &looperMailbox = getCommandMailbox();
        looperMailbox.tryPush(looper::LooperCommand::resume(trackIndex));
    }

    void Looper::clearAll()
    {
        auto &looperMailbox = getCommandMailbox();
        looperMailbox.tryPush(LooperCommand::clearAllTracks());
    }

    bool Looper::sendMidiMessage(const midi::MidiMessage& message)
    {
        return cb_->midiQueue.tryPush(message);
    }

    LooperMailbox& Looper::getCommandMailbox() noexcept
    {
        return cb_->looper.getSharedData().commandMailbox;
    }

}
