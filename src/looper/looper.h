#pragma once

#include <memory>
#include <string>

#include "looper_processor.h"
#include "midi/midi_message.h"

namespace looper {
    struct TrackStateSnapshot
    {
        unsigned int nFrames;
        unsigned int position;
        State state;

        [[nodiscard]] std::string toString() const;
    };

    struct LooperStateSnapshot
    {
        std::array<TrackStateSnapshot, kLooperTrackCount> tracks;
    };

    class Looper
    {
    public:
        Looper();
        ~Looper() = default;

        Looper(const Looper &) = delete;
        Looper& operator=(const Looper &) = delete;

        Looper(const Looper &&) = delete;
        Looper& operator=(Looper &&) = delete;

        [[nodiscard]] int getNumLooperTracks() const noexcept;

        void updateSnapshot() noexcept;
        [[nodiscard]] const TrackStateSnapshot& getTrackState(int trackIndex) const noexcept;

        void startRecording(int trackIndex);
        void stopRecording(int trackIndex);
        void clear(int trackIndex);
        void pause(int trackIndex);
        void resume(int trackIndex);
        void clearAll();

        bool sendMidiMessage(const midi::MidiMessage& message);

    private:
        LooperMailbox& getCommandMailbox() noexcept;

        class LooperCallback;
        std::shared_ptr<LooperCallback> cb_;
        LooperStateSnapshot snapshot_{};
    };
}