#pragma once

#include <atomic>
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

    enum class State : unsigned char
    {
        CLEARED,
        RECORDING,
        PLAYBACK,
    };

    State getState(int trackIndex) const noexcept;
    unsigned int getCurrentPosition(int trackIndex) const noexcept;
    unsigned int getCurrentNumFrames(int trackIndex) const noexcept;
    bool isEmpty(int trackIndex) const noexcept;

    void startRecording(int trackIndex) noexcept;
    void stopRecording(int trackIndex) noexcept;
    void clear(int trackIndex) noexcept;

    void clearAll() noexcept;

    static const char* stateToStr(State state);

private:
    static constexpr unsigned int MAX_LOOP_LENGTH_IN_SECONDS = 30;


    bool isTrackIndexValid(int trackIndex) const noexcept;
    void consumeCommands() noexcept;
    void processInternal(float *const *data, unsigned int nFrames) noexcept;

    RelaxedAtomic<State> state_{State::CLEARED};
    RelaxedAtomic<unsigned int> position_{0};
    RelaxedAtomic<unsigned int> numFrames_{0};

    unsigned int numChannels_{0};
    unsigned int maxFrames_{0};
    std::vector<std::vector<float>> buffers_;

    LooperMailbox commandMailbox_{128};
};

}
