#include "looper.h"

#include <format>
#include <cmath>

#include "audio/audio_engine.h"
#include "looper_processor.h"
#include "looper_commands.h"
#include "midi/foot_switch.h"
#include "threading/triple_buffer.h"
#include "dsp/level_meter.h"
#include "source_mixer.h"

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

    LooperSessionData::LooperSessionData(const unsigned int maxFrames)
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
        void onProcess(const float *const *in, float *const *out, const unsigned int nFrames) override
        {
            fsTrackIndex_ = fsTrackParam.asParameterUnsafe().get<int>();

            for (auto i{0u}; i < nFrames; ++i) {
                footSwitch.tick();
            }

            drainMidiQueue();
            consumeCommands();

            sourceMixer.process(in, out, nFrames);
            looperProcessor.process(out, nFrames);

            levelMeter_(out[0], out[1], nFrames);

            const auto headroomScalar = dsp::dBtoLinear(-kHeadRoomDb);
            for (auto ch{0u}; ch < 2; ++ch) {
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
                for (auto c{0u}; c < 2; ++c) {
                    out[c][i] = sine;
                }
            }*/
        }

        void onStart() override
        {
            looperProcessor.onStart();
            // ensure mailbox is clear from stale messages
            consumeCommands();
            looperProcessor.clearAll();
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

            assert(audio::AudioEngine::getInstance().getNumOutputChannels() >= 2);

            const auto sampleRate = audio::AudioEngine::getInstance().getSampleRate();
            const auto nInputs = audio::AudioEngine::getInstance().getNumInputChannels();

            sourceMixer.prepare(nInputs, audio::kMaxFramesInBuffer, sampleRate);
            levelMeter_.prepare(static_cast<float>(sampleRate));
        }

        void onStop() override
        {
            looperProcessor.onStop();
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
                cmd.apply(looperProcessor);
            });
        }

        void updateSnapshot() noexcept
        {
            auto writer = sharedData.state.getWriter();
            auto &snapshot = writer.data();

            for (auto i{0}; i < LooperProcessor::getNumLooperTracks(); ++i) {
                auto& [nFrames, position, state, level] = snapshot.tracks[i];
                nFrames = looperProcessor.getCurrentNumFrames(i);
                position = looperProcessor.getCurrentPosition(i);
                state = looperProcessor.getState(i);
                level = looperProcessor.getMixer().getLevel(i);
            }

            snapshot.maxLoopLength = looperProcessor.getMaxFramesInLoop();
            snapshot.level = levelMeter_.getLevel();
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
            {0, LooperProcessor::getNumLooperTracks() - 1}
        )};

    private:
        int fsTrackIndex_{0};
    };

    Looper::Looper()
        : cb_(std::make_shared<LooperCallback>())
        , paramTree_("Looper")
    {
        paramTree_.addSubTree(cb_->sourceMixer.getParameterTree());
        paramTree_.addSubTree(cb_->looperProcessor.getMixer().getParameterTree());
        paramTree_.addSubTree(cb_->fsTrackParam);

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
        getCommandMailbox().waitPush(cmd);
        while (!completionFlag.complete) {}
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

        if (audio::AudioEngine::getInstance().isRunning()) {
            getCommandMailbox().waitPush(cmd);
            while (!completionFlag.complete) {
                //std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
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