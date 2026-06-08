#pragma once

#include <memory>
#include <string>

#include "audio/audio_engine.h"
#include "dsp/parameter/parameter_tree.h"
#include "looper_commands.h"
#include "looper_processor.h"
#include "midi/midi_message.h"

namespace looper {
    inline constexpr float kHeadRoomDb = 6.0f;

    struct TrackStateSnapshot
    {
        FrameInt nFrames;
        FrameInt position;
        State state;
        std::pair<float, float> level;

        [[nodiscard]] std::string toString() const;
    };

    struct LooperStateSnapshot
    {
        std::array<TrackStateSnapshot, kLooperTrackCount> tracks;
        FrameInt maxLoopLength{};
        std::pair<float, float> level;
    };

    struct LooperSessionData
    {
        static constexpr auto kCount = kLooperTrackCount;
        float *leftBuffers[kLooperTrackCount]{};
        float *rightBuffers[kLooperTrackCount]{};
        FrameInt frameCounts[kLooperTrackCount]{};

        explicit LooperSessionData(FrameInt maxFrames);

    private:
        using SampleBuffer = std::vector<float>;
        std::array<std::pair<SampleBuffer, SampleBuffer>, kLooperTrackCount> buffers_;
    };

    class Looper
    {
    public:
        Looper(audio::AudioEngine &audioEngine);
        ~Looper() = default;

        Looper(const Looper &) = delete;
        Looper& operator=(const Looper &) = delete;

        Looper(const Looper &&) = delete;
        Looper& operator=(Looper &&) = delete;

        [[nodiscard]] int getNumLooperTracks() const noexcept;

        // Call once per ui frame
        void updateSnapshot() noexcept;

        [[nodiscard]] const TrackStateSnapshot& getTrackState(int trackIndex) const noexcept;
        [[nodiscard]] const LooperStateSnapshot& getLooperState() const noexcept;
        [[nodiscard]] dsp::parameter::ParameterTree getParameterTree() const noexcept;

        [[nodiscard]] json getSettingsAsJson() const;
        bool loadSettingsFromJson(const json& j);

        bool toggleRecording(int trackIndex, bool synced = true) const;
        bool togglePlay(int trackIndex, bool synced = true) const;
        void startRecording(int trackIndex, bool synced = true) const;
        void stopRecording(int trackIndex, bool synced = true) const;
        void clear(int trackIndex) const;
        void pause(int trackIndex, bool synced = true) const;
        void resume(int trackIndex, bool synced = true) const;
        void clearAll() const;
        void getThumbnail(int trackIndex, ThumbnailSnapshot& out) const noexcept;

        [[nodiscard]] std::unique_ptr<LooperSessionData> getSessionData(bool isAudioThreadRunning = true) const;

        [[nodiscard]] bool sendMidiMessage(const midi::MidiMessage& message) const;

    private:
        [[nodiscard]] LooperMailbox& getCommandMailbox() const noexcept;

        class LooperCallback;
        std::shared_ptr<LooperCallback> cb_;
        LooperStateSnapshot snapshot_{};

        dsp::parameter::ParameterTree paramTree_;
    };
}