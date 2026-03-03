#include "looper.h"

#include <format>

#include "audio/audio_engine.h"
#include "looper_processor.h"
#include "looper_commands.h"
#include "midi/foot_switch.h"
#include "spsc_mailbox.h"
#include "triple_buffer.h"

namespace looper {
    struct LooperSharedData
    {
        LooperMailbox commandMailbox{128};
        TripleBuffer<LooperStateSnapshot> state;
    };

    std::string TrackStateSnapshot::toString() const
    {
        return std::format("nFrames: {}, position: {}, state: {}", nFrames, position, stateToStr(state));
    }

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

                footSwitch.tick();
            }

            drainMidiQueue();
            consumeCommands();

            looper.process(out, nFrames);

            updateSnapshot();

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
            looper.onStart();
            // ensure mailbox is clear from stale messages
            consumeCommands();
            looper.clearAll();
            updateSnapshot();

            constexpr int trackIndex = 0;

            footSwitch.setOnSinglePressed([&] {
                if (looper.getState(trackIndex) != looper::State::Recording) {
                    looper.startRecording(trackIndex);
                } else {
                    looper.stopRecording(trackIndex);
                }
            });

            footSwitch.setOnDoublePressed([&] {
                looper.clear(trackIndex);
            });

            footSwitch.setOnHold([&] {
                looper.clearAll();
            });
        }

        void onStop() override
        {
            looper.onStop();
        }

        void drainMidiQueue()
        {
            midiQueue.consumeAll([&](const midi::MidiMessage& msg) {
                footSwitch.update(msg);
            });
        }

        void consumeCommands() noexcept
        {
            sharedData.commandMailbox.consumeAll([&](const LooperCommand& cmd) {
                cmd.apply(looper);
            });
        }

        void updateSnapshot() noexcept
        {
            auto writer = sharedData.state.getWriter();
            auto& snapshot = writer.data();

            for (auto i{0}; i < LooperProcessor::getNumLooperTracks(); ++i) {
                auto& [nFrames, position, state] = snapshot.tracks[i];
                nFrames = looper.getCurrentNumFrames(i);
                position = looper.getCurrentPosition(i);
                state = looper.getState(i);
            }
        }

        looper::LooperProcessor looper;
        looper::LooperSharedData sharedData;

        midi::MidiQueue midiQueue{64};
        midi::FootSwitch footSwitch;
    };

    Looper::Looper() : cb_(std::make_shared<LooperCallback>())
    {
        auto& engine = audio::AudioEngine::getInstance();
        engine.setAudioCallback(cb_);
    }

    int Looper::getNumLooperTracks() const noexcept
    {
        return LooperProcessor::getNumLooperTracks();
    }

    void Looper::updateSnapshot() noexcept
    {
        const auto reader = cb_->sharedData.state.read();
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
        return cb_->sharedData.commandMailbox;
    }
}