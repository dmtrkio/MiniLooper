#pragma once

#include <array>
#include <vector>
#include <string>

#include "looper_commands.h"
#include "triple_buffer.h"

namespace looper {

    constexpr unsigned int kMaxLoopSecs = 32;
    constexpr unsigned int kLooperTrackCount{4};

    enum class State
    {
        Cleared,
        Recording,
        Playback,
        Paused,
    };

    const char* stateToStr(State state);

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

    struct LooperSharedData
    {
        LooperMailbox commandMailbox{128};
        TripleBuffer<LooperStateSnapshot> state;
    };

    class LooperProcessor
    {
    public:
        // lifetime callbacks
        void process(float *const *data, unsigned int nFrames) noexcept;
        void onStart();
        void onStop();

        static constexpr int getNumLooperTracks() { return kLooperTrackCount; }

        LooperSharedData& getSharedData() noexcept;

        // All the methods below are not thread-safe, they are meant to be used in the same thread where process() is called

        State getState(int trackIndex) const noexcept;
        unsigned int getCurrentPosition(int trackIndex) const noexcept;
        unsigned int getCurrentNumFrames(int trackIndex) const noexcept;
        bool isEmpty(int trackIndex) const noexcept;

        void startRecording(int trackIndex) noexcept;
        void stopRecording(int trackIndex) noexcept;
        void clear(int trackIndex) noexcept;
        void pause(int trackIndex) noexcept;
        void resume(int trackIndex) noexcept;
        void clearAll() noexcept;

    private:
        unsigned int getNextGridDivision(int frameIndex) const noexcept;
        bool isTrackIndexValid(int trackIndex) const noexcept;
        bool isAnyTrackCurrentlyRecording() const noexcept;

        void updateSnapshot() noexcept;
        void consumeCommands() noexcept;
        void processInternal(float *const *data, unsigned int nFrames) noexcept;
        void processTrack(int trackIndex, float *const *data, unsigned int nFrames) noexcept;

        static constexpr std::array<float, 5> kGridMultipliers = { 1.0f, 2.0f, 4.0f, 8.0f, 16.0f };

        struct Transport
        {
            [[nodiscard]] bool isTempoSet() const noexcept;
            void tick(unsigned int nFrames) noexcept;
            void setBarLength(unsigned int nFrames, unsigned int maxFrames) noexcept;
            void reset(unsigned int maxFrames) noexcept;

            std::uint64_t currentFrame{};
            unsigned int barLength{};
            unsigned int largestPossibleLoopLength{};
        };

        Transport transport_;

        unsigned int numChannels_{0};
        unsigned int maxFrames_{0};

        struct Track
        {
            void init(int index, unsigned int numChannels, unsigned int maxFrames) noexcept;
            [[nodiscard]] bool isEmpty() const noexcept;
            void scheduleTransition(State next, unsigned int when);
            void transitionState(State newState, unsigned int transportFrame) noexcept;
            [[nodiscard]] unsigned int phase(unsigned int transportFrame) const noexcept;

            /*
            void advance(Transport &transport, unsigned int maxFrames) noexcept;
            [[nodiscard]] float read(unsigned int channel) const noexcept;
            void writeAdding(unsigned int channel, float value) noexcept;
            void overwrite(unsigned int channel, float value) noexcept;
            */

            int trackIndex{-1};
            State state{State::Cleared};
            unsigned int start{0};
            unsigned int length{0};

            unsigned int crossfadeLength{64};
            std::vector<std::vector<float>> buffers;

            State pendingState{State::Cleared};
            bool hasPendingTransition{false};
            int framesToTransition{0};
        };

        std::array<Track, kLooperTrackCount> tracks_{};

        std::vector<std::vector<float>> sumBuffers_;

        LooperSharedData sharedData_{};
    };

}
