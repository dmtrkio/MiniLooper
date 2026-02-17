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

    enum class State : unsigned char
    {
        CLEARED,
        RECORDING,
        PLAYBACK,
    };

    LooperMailbox& getCommandMailbox() noexcept;
    State getState() const noexcept;
    unsigned int getCurrentPosition() const noexcept;
    unsigned int getCurrentNumFrames() const noexcept;
    bool isEmpty() const noexcept;

    void startRecording() noexcept;
    void stopRecording() noexcept;
    void clear() noexcept;

    static const char* stateToStr(State state);

private:
    static constexpr unsigned int MAX_LOOP_LENGTH_IN_SECONDS = 30;

    void consumeCommands() noexcept;
    void processInternal(float *const *data, unsigned int nFrames) noexcept;

    RelaxedAtomic<State> state_{State::CLEARED};
    std::atomic<unsigned int> position_{0};
    std::atomic<unsigned int> numFrames_{0};

    unsigned int numChannels_{0};
    unsigned int maxFrames_{0};
    std::vector<std::vector<float>> buffers_;

    LooperMailbox commandMailbox_{128};
};

}
