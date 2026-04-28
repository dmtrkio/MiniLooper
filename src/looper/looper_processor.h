#pragma once

#include <array>
#include <vector>
#include <utility>

#include "looper_mixer.h"

namespace looper {
    constexpr unsigned int kMaxLoopSecs = 32;
    constexpr unsigned int kLooperTrackCount = 4;
    constexpr float kFadeLengthMs = 5.0f;

    enum class State
    {
        Cleared,
        Recording,
        Playback,
        Paused,
    };

    const char* stateToStr(State state);

    class LooperProcessor
    {
    public:
        LooperProcessor();

        // lifetime callbacks
        void process(float *const *data, unsigned int nFrames) noexcept;
        void onStart();
        void onStop();

        static constexpr int getNumLooperTracks() { return kLooperTrackCount; }

        // All the methods below are not thread-safe, they are meant to be used in the same thread where process() is called

        [[nodiscard]] Mixer& getMixer() noexcept;
        [[nodiscard]] State getState(int trackIndex) const noexcept;
        [[nodiscard]] unsigned int getCurrentPosition(int trackIndex) const noexcept;
        [[nodiscard]] unsigned int getCurrentNumFrames(int trackIndex) const noexcept;
        [[nodiscard]] bool isEmpty(int trackIndex) const noexcept;

        void startRecording(int trackIndex) noexcept;
        void stopRecording(int trackIndex) noexcept;
        void clear(int trackIndex) noexcept;
        void pause(int trackIndex) noexcept;
        void resume(int trackIndex) noexcept;
        void clearAll() noexcept;
        [[nodiscard]] unsigned int copyLoop(int trackIndex, float *const *data, unsigned int capacity) const noexcept;

    private:
        [[nodiscard]] unsigned int getNextGridDivision(int frameIndex) const noexcept;
        static constexpr bool isTrackIndexValid(int trackIndex);
        [[nodiscard]] bool isAnyTrackCurrentlyRecording() const noexcept;

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
            [[nodiscard]] std::pair<float, float> getFadeScalars(unsigned int pos) const noexcept;

            int trackIndex{-1};
            State state{State::Cleared};
            unsigned int start{0};
            unsigned int length{0};

            unsigned int fadeLength{64};

            std::vector<std::vector<float>> buffers;

            State pendingState{State::Cleared};
            bool hasPendingTransition{false};
            int framesToTransition{0};
        };

        std::array<Track, kLooperTrackCount> tracks_{};

        Mixer mixer_;
        std::vector<std::vector<float>> sumBuffers_;
    };
}
