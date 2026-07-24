#include "looper.h"

#include <format>
#include <cmath>

#include "audio/audio_engine.h"
#include "dsp/dsp.h"
#include "dsp/effects/gain_effect.h"
#include "looper_processor.h"
#include "looper_commands.h"
#include "midi/foot_switch.h"
#include "threading/triple_buffer.h"
#include "dsp/level_meter.h"
#include "source_mixer.h"

namespace ml::looper {
    struct LooperSharedData
    {
        LooperMailbox commandMailbox{128};
        TripleBuffer<LooperStateSnapshot> state;
    };

    std::string TrackStateSnapshot::toString() const
    {
        return std::format("nFrames: {}, position: {}, state: {}", nFrames, position, stateToStr(state));
    }

    LooperSessionData::LooperSessionData(const FrameInt maxFrames)
    {
        for (auto i{0u}; i < kLooperTrackCount; ++i) {
            buffers_[i].first.resize(maxFrames);
            buffers_[i].second.resize(maxFrames);
            leftBuffers[i] = buffers_[i].first.data();
            rightBuffers[i] = buffers_[i].second.data();
            frameCounts[i] = 0;
        }
    }

    class Looper::LooperCallback final : public audio::AudioCallback
    {
    public:
        LooperCallback()
        {
            masterGain.rename("MasterGain");
        }

        void onProcess(const float *const *in, float *const *out, const int nFrames) override
        {
            fsTrackIndex_ = fsTrackParam.asParameterUnsafe().get<int>();

            for (int i{0}; i < nFrames; ++i) {
                footSwitch.tick();
            }

            drainMidiQueue();
            consumeCommands();

            //const float clickGain = dsp::dBtoLinear(clickParamTree["Gain"].asParameterUnsafe().get<float>());
            looperProcessor.setClickGain(dsp::dBtoLinear(-3.0f));

            const bool clickEnabled = clickParamTree["Enabled"].asParameterUnsafe().get<bool>();
            looperProcessor.setClickEnabled(clickEnabled);

            sourceMixer.process(in, out, nFrames);
            looperProcessor.process(out, static_cast<FrameInt>(nFrames));
            masterGain.process(out, nFrames);

            levelMeter_(out[0], out[1], nFrames);

            const auto headroomScalar = dsp::dBtoLinear(-kHeadRoomDb);
            for (auto ch{0u}; ch < 2; ++ch) {
                for (int i{0}; i < nFrames; ++i) {
                    out[ch][i] = std::tanh(out[ch][i] * headroomScalar);
                }
            }

            updateSnapshot();
        }

        void onStart(float sampleRate, int nInputChannels, int nOutputChannels) override
        {
            assert(nOutputChannels >= 2);
            (void)nOutputChannels;

            if (sessionToLoad) {
                looperProcessor.prepare(sampleRate, &sessionToLoad.value());
            } else {
                looperProcessor.prepare(sampleRate);
            }

            sessionToLoad.reset();

            // ensure mailbox is clear from stale messages
            consumeCommands();
            updateSnapshot();

            footSwitch.setOnSinglePressed([&] {
                if (looperProcessor.getState(fsTrackIndex_) != looper::State::Recording) {
                    looperProcessor.startRecording(fsTrackIndex_, false);
                } else {
                    looperProcessor.stopRecording(fsTrackIndex_, false);
                }
            });

            footSwitch.setOnDoublePressed([&] {
                looperProcessor.clear(fsTrackIndex_);
            });

            footSwitch.setOnHold([&] {
                looperProcessor.clearAll();
            });

            sourceMixer.prepare(nInputChannels, audio::kMaxFramesInBuffer, sampleRate);
            levelMeter_.prepare(sampleRate);
            masterGain.prepare(sampleRate);
            masterGain.setEnabled(true);
        }

        void onStop() override {}

        void drainMidiQueue()
        {
            midiQueue.consumeAll([&](const midi::MidiMessage& msg) {
                if (msg.isNoteOn()) {
                    int channel = -1;
                    switch (msg.note()) {
                        case 60: channel = 0; break;
                        case 62: channel = 1; break;
                        case 64: channel = 2; break;
                        case 65: channel = 3; break;
                        default: return;
                    }

                    if (looperProcessor.getState(channel) != looper::State::Recording) {
                        looperProcessor.startRecording(channel, false);
                    } else {
                        looperProcessor.stopRecording(channel, false);
                    }
                }

                footSwitch.update(msg);
            });
        }

        void consumeCommands() noexcept
        {
            sharedData.commandMailbox.consumeAll([&](const LooperCommand& cmd) {
                cmd.apply(looperProcessor);
            });
        }

        void updateSnapshot() noexcept
        {
            auto writer = sharedData.state.getWriter();
            auto &snapshot = writer.data();

            for (auto i{0}; i < looperProcessor.getNumLooperTracks(); ++i) {
                auto& [nFrames, position, offset, state, level] = snapshot.tracks[i];
                nFrames = looperProcessor.getCurrentNumFrames(i);
                position = looperProcessor.getCurrentPosition(i);
                offset = looperProcessor.getRelativeOffset(i);
                state = looperProcessor.getState(i);
                level = looperProcessor.getMixer().getLevel(i);
            }

            snapshot.isTransportRunning = looperProcessor.isTransportRunning();
            snapshot.maxLoopLength = looperProcessor.getMaxFramesInLoop();
            snapshot.beatLength = looperProcessor.getBeatLength();
            snapshot.level = levelMeter_.getLevel();
            snapshot.sourceChannelLevels = sourceMixer.getLevels();
        }

        LooperProcessor looperProcessor;
        LooperSharedData sharedData;

        midi::MidiQueue midiQueue{64};
        midi::FootSwitch footSwitch;
        SourceMixer sourceMixer;

        dsp::LevelMeter levelMeter_;

        dsp::parameter::ParameterTree fsTrackParam{dsp::parameter::Parameter::makeInteger(
            "FootSwitchTrackIndex",
            0,
            {0, kLooperTrackCount - 1}
        )};

        dsp::parameter::ParameterTree clickParamTree{"Click", {
            dsp::parameter::Parameter::makeFloat("Gain", -6.0f, {-60.f, 12.0f}),
            dsp::parameter::Parameter::makeBoolean("Enabled", true)
        }};

        dsp::effects::GainEffect masterGain;

        std::optional<LooperSession> sessionToLoad;

    private:
        void rotateTrackIndex()
        {
            fsTrackParam.asParameterUnsafe().set((fsTrackIndex_ + 1) % kLooperTrackCount);
        }

        int fsTrackIndex_{0};
    };

    Looper::Looper(audio::AudioEngine &audioEngine)
        : audioEngine_(&audioEngine)
        , cb_(std::make_shared<LooperCallback>())
        , paramTree_("Looper")
    {
        paramTree_.addSubTree(cb_->sourceMixer.getParameterTree());
        paramTree_.addSubTree(cb_->looperProcessor.getMixer().getParameterTree());
        paramTree_.addSubTree(cb_->fsTrackParam);
        paramTree_.addSubTree(cb_->clickParamTree);
        paramTree_.addSubTree(cb_->masterGain.getParameterTree());

        audioEngine.setAudioCallback(cb_);
    }

    int Looper::getNumLooperTracks() const noexcept
    {
        return cb_->looperProcessor.getNumLooperTracks();
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

    dsp::parameter::ParameterTree Looper::getParameterTree() const noexcept
    {
        return paramTree_;
    }

    json Looper::getSettingsAsJson() const
    {
        return getParameterTree().toJson();
    }

    bool Looper::loadSettingsFromJson(const json& j)
    {
        return paramTree_.copyParameterValuesFromJson(j);
    }

    void Looper::loadSession(LooperSession&& session)
    {
        cb_->sessionToLoad = std::move(session);
    }

    void Looper::pauseTransport() noexcept
    {
        auto &looperMailbox = getCommandMailbox();
        looperMailbox.tryPush(looper::LooperCommand::pauseTransport());
    }

    void Looper::playTransport() noexcept
    {
        auto &looperMailbox = getCommandMailbox();
        looperMailbox.tryPush(looper::LooperCommand::playTransport());
    }

    void Looper::stopTransport() noexcept
    {
        auto &looperMailbox = getCommandMailbox();
        looperMailbox.tryPush(looper::LooperCommand::stopTransport());
    }

    bool Looper::toggleRecording(int trackIndex, bool synced) const
    {
        if (getTrackState(trackIndex).state != looper::State::Recording) {
            startRecording(trackIndex, synced);
            return true;
        } else {
            stopRecording(trackIndex, synced);
            return false;
        }
    }

    bool Looper::togglePlay(int trackIndex, bool synced) const
    {
        if (getTrackState(trackIndex).state == looper::State::Paused) {
            resume(trackIndex, synced);
            return true;
        } else {
            pause(trackIndex, synced);
            return false;
        }
    }

    void Looper::startRecording(int trackIndex, bool synced) const
    {
        auto &looperMailbox = getCommandMailbox();
        looperMailbox.tryPush(looper::LooperCommand::startRecording(trackIndex, synced));
    }

    void Looper::stopRecording(int trackIndex, bool synced) const
    {
        auto &looperMailbox = getCommandMailbox();
        looperMailbox.tryPush(looper::LooperCommand::stopRecording(trackIndex, synced));
    }

    void Looper::clear(int trackIndex) const
    {
        auto &looperMailbox = getCommandMailbox();
        looperMailbox.tryPush(looper::LooperCommand::clear(trackIndex));
    }

    void Looper::pause(int trackIndex, bool synced) const
    {
        auto &looperMailbox = getCommandMailbox();
        looperMailbox.tryPush(looper::LooperCommand::pause(trackIndex, synced));
    }

    void Looper::resume(int trackIndex, bool synced) const
    {
        auto &looperMailbox = getCommandMailbox();
        looperMailbox.tryPush(looper::LooperCommand::resume(trackIndex, synced));
    }

    void Looper::clearAll() const
    {
        auto &looperMailbox = getCommandMailbox();
        looperMailbox.tryPush(LooperCommand::clearAllTracks());
    }

    void Looper::getThumbnail(int trackIndex, ThumbnailSnapshot& out) const noexcept
    {
        LooperCommand::CompletionFlag completionFlag;
        const auto cmd = LooperCommand::getThumbnail(trackIndex, out, completionFlag);

        if (audioEngine_->isRunning()) {
            getCommandMailbox().waitPush(cmd);
            while (!completionFlag.complete) {}
        } else {
            cmd.apply(cb_->looperProcessor);
        }
    }

    std::unique_ptr<LooperSessionData> Looper::getSessionData() const
    {
        const auto maxFrames = cb_->looperProcessor.getMaxFramesInLoop();

        auto session = std::make_unique<LooperSessionData>(maxFrames);

        LooperCommand::CopyData copyData = {
            .maxFrames = maxFrames,
            .buffersL = session->leftBuffers,
            .buffersR = session->rightBuffers,
            .framesWritten = session->frameCounts,
        };

        LooperCommand::CompletionFlag completionFlag;
        const auto cmd = LooperCommand::copyLoops(copyData, completionFlag);

        if (audioEngine_->isRunning()) {
            getCommandMailbox().waitPush(cmd);
            while (!completionFlag.complete) {}
        } else {
            cmd.apply(cb_->looperProcessor);
        }

        return session;
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