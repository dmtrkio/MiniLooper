#include "looper.h"

#include <format>
#include <cmath>
#include <thread>

#include "audio/audio_engine.h"
#include "audio/wav_writer.h"
#include "looper_processor.h"
#include "looper_commands.h"
#include "midi/foot_switch.h"
#include "spsc_mailbox.h"
#include "triple_buffer.h"
#include "dsp/level_meter.h"

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
            assert(oChannels >= 2);

            for (auto i{0u}; i < nFrames; ++i) {
                float inputSample = 0.0f;
                for (auto c{0u}; c < iChannels; ++c) {
                    inputSample += in[c][i];
                }
                for (auto c{0u}; c < oChannels; ++c) {
                    out[c][i] = inputSample;
                }
            }

            for (auto i{0u}; i < nFrames; ++i) {
                footSwitch.tick();
            }

            drainMidiQueue();
            consumeCommands();

            looper.process(out, nFrames);

            levelMeter_(out[0], out[1], nFrames);

            const auto headroomScalar = dsp::dBtoLinear(-kHeadRoomDb);
            for (auto ch{0u}; ch < oChannels; ++ch) {
                for (auto i{0u}; i < nFrames; ++i) {
                    out[ch][i] = std::tanh(out[ch][i] * headroomScalar);
                }
            }

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

            const auto sampleRate = static_cast<float>(audio::AudioEngine::getInstance().getSampleRate());
            levelMeter_.prepare(sampleRate);
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
            auto &snapshot = writer.data();

            for (auto i{0}; i < LooperProcessor::getNumLooperTracks(); ++i) {
                auto& [nFrames, position, state, level] = snapshot.tracks[i];
                nFrames = looper.getCurrentNumFrames(i);
                position = looper.getCurrentPosition(i);
                state = looper.getState(i);
                level = looper.getMixer().channels[i].meter.getLevel();
            }

            snapshot.level = levelMeter_.getLevel();
        }

        LooperProcessor looper;
        LooperSharedData sharedData;

        midi::MidiQueue midiQueue{64};
        midi::FootSwitch footSwitch;

        dsp::LevelMeter levelMeter_;
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

    const LooperStateSnapshot& Looper::getLooperState() const noexcept
    {
        return snapshot_;
    }

    MixerParams& Looper::getMixerParams() const noexcept
    {
        return cb_->looper.getMixer().params;
    }

    void Looper::startRecording(int trackIndex) const
    {
        auto &looperMailbox = getCommandMailbox();
        looperMailbox.tryPush(looper::LooperCommand::startRecording(trackIndex));
    }

    void Looper::stopRecording(int trackIndex) const
    {
        auto &looperMailbox = getCommandMailbox();
        looperMailbox.tryPush(looper::LooperCommand::stopRecording(trackIndex));
    }

    void Looper::clear(int trackIndex) const
    {
        auto &looperMailbox = getCommandMailbox();
        looperMailbox.tryPush(looper::LooperCommand::clear(trackIndex));
    }

    void Looper::pause(int trackIndex) const
    {
        auto &looperMailbox = getCommandMailbox();
        looperMailbox.tryPush(looper::LooperCommand::pause(trackIndex));
    }

    void Looper::resume(int trackIndex) const
    {
        auto &looperMailbox = getCommandMailbox();
        looperMailbox.tryPush(looper::LooperCommand::resume(trackIndex));
    }

    void Looper::clearAll() const
    {
        auto &looperMailbox = getCommandMailbox();
        looperMailbox.tryPush(LooperCommand::clearAllTracks());
    }

    void Looper::saveToDisk() const
    {
        const auto sampleRate = audio::AudioEngine::getInstance().getSampleRate();
        const auto maxFrames = sampleRate * kMaxLoopSecs;

        using SampleBuffer = std::vector<float>;
        std::array<std::pair<SampleBuffer, SampleBuffer>, kLooperTrackCount> buffers;
        float *leftBuffers[kLooperTrackCount];
        float *rightBuffers[kLooperTrackCount];
        unsigned int framesWritten[kLooperTrackCount];

        for (auto i{0u}; i < kLooperTrackCount; ++i) {
            buffers[i].first.resize(maxFrames);
            buffers[i].second.resize(maxFrames);
            leftBuffers[i] = buffers[i].first.data();
            rightBuffers[i] = buffers[i].second.data();
            framesWritten[i] = 0;
        }

        LooperCommand::CopyData copyData = {
            .maxFrames = maxFrames,
            .buffersL = leftBuffers,
            .buffersR = rightBuffers,
            .framesWritten = framesWritten,
        };

        LooperCommand::CompletionFlag completionFlag;
        getCommandMailbox().waitPush(LooperCommand::copyLoops(&copyData, &completionFlag));
        while (!completionFlag.complete) {
            //std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        for (int i = 0; i < getNumLooperTracks(); ++i) {
            float *data[2] = { leftBuffers[i], rightBuffers[i] };
            if (const auto framesToWrite = copyData.framesWritten[i]; framesToWrite > 0) {
                const auto fileName = std::format("looper_track_{}.wav", i);
                audio::WavWriter wavWriter(fileName.c_str(), sampleRate, 2);
                wavWriter.writeFrames(data, framesToWrite);
            }
        }
    }

    bool Looper::sendMidiMessage(const midi::MidiMessage& message) const
    {
        return cb_->midiQueue.tryPush(message);
    }

    LooperMailbox& Looper::getCommandMailbox() const noexcept
    {
        return cb_->sharedData.commandMailbox;
    }
}