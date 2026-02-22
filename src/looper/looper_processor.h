#pragma once

#include <array>
#include <atomic>
#include <iostream>
#include <vector>

#include "atomic_wrapper.h"
#include "looper_commands.h"

namespace looper {

class LooperProcessor
{
public:
    void process(float *const *data, unsigned int nFrames) noexcept;
    void onStart();
    void onStop();

    int getNumLooperTracks() const noexcept;
    LooperMailbox& getCommandMailbox() noexcept;

    static constexpr unsigned int MAX_LOOP_LENGTH_IN_SECONDS = 16;

    enum class State
    {
        CLEARED,
        RECORDING,
        PLAYBACK,
        PAUSED,
    };

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

    static const char* stateToStr(State state);

private:
    unsigned int getNextGridDivision(int frameIndex) const noexcept;
    bool isTrackIndexValid(int trackIndex) const noexcept;
    bool isAnyTrackCurrentlyRecording() const noexcept;
    void consumeCommands() noexcept;
    void processInternal(float *const *data, unsigned int nFrames) noexcept;
    void processTrack(int trackIndex, float *const *data, unsigned int nFrames) noexcept;

    static constexpr unsigned int NUM_LOOPER_TRACKS{4};
    static constexpr std::array<float, 4> GRID_MULTIPLIERS = { 1.0f, 2.0f, 4.0f, 8.0f };

    struct Transport
    {
        [[nodiscard]] bool isTempoSet() const noexcept { return barLength != 0; }

        void tick(unsigned int numFrames) noexcept
        {
            if (isTempoSet()) currentFrame += numFrames;
        }

        void setBarLength(unsigned int nFrames, unsigned int maxFrames) noexcept
        {
            currentFrame = 0;
            barLength = nFrames;

            if (nFrames == 0) {
                largestPossibleLoopLength = maxFrames;
                return;
            }

            largestPossibleLoopLength = barLength;

            for (int i = GRID_MULTIPLIERS.size() - 1; i >= 0; --i) {
                const auto target = static_cast<unsigned int>(GRID_MULTIPLIERS[i] * static_cast<float>(barLength));
                if (target < maxFrames) {
                    largestPossibleLoopLength = target;
                    break;
                }
            }
        }

        void reset(unsigned int maxFrames) noexcept
        {
            setBarLength(0, maxFrames);
        }

        unsigned int currentFrame{};
        unsigned int barLength{};
        unsigned int largestPossibleLoopLength{};
    };

    Transport transport_;

    unsigned int numChannels_{0};
    unsigned int maxFrames_{0};

    struct TransitionTimer
    {
        void reset() noexcept
        {
            hasNext = false;
            nextState = State::CLEARED;
            framesLeft = 0;
        }

        bool hasNext{false};
        State nextState{State::CLEARED};
        unsigned int framesLeft{0};
    };

    struct Track
    {
        void init(unsigned int numChannels, unsigned int maxFrames) noexcept;
        bool tick();
        void scheduleTransition(State next, unsigned int when);

        RelaxedAtomic<State> state{State::CLEARED};
        RelaxedAtomic<unsigned int> position{0};
        RelaxedAtomic<unsigned int> nFrames{0};

        std::vector<std::vector<float>> buffers;

        TransitionTimer transitionTimer;
    };

    std::array<Track, NUM_LOOPER_TRACKS> tracks_{};

    std::vector<std::vector<float>> sumBuffers_;

    LooperMailbox commandMailbox_{128};
};

}
