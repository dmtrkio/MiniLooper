#pragma once

#include <vector>

#include "looper_commands.h"

namespace looper {

class Looper
{
public:
    void process(float **data, unsigned int nFrames) noexcept;
    void onStart();
    void onStop();

    LooperMailbox& getCommandMailbox() noexcept;

    void startRecording() noexcept;
    void stopRecording() noexcept;
    void clear() noexcept;

private:
    void consumeCommands() noexcept;
    void processInternal(float **data, unsigned int nFrames) noexcept;

    enum class State
    {
        CLEARED,
        RECORDING,
        PLAYBACK,
    };

    State state_{State::CLEARED};
    unsigned int position_{0};
    unsigned int numFrames_{0};

    unsigned int maxLengthInSec_{10};
    unsigned int numChannels_{0};
    unsigned int maxFrames_{0};
    std::vector<std::vector<float>> buffers_;

    LooperMailbox commandMailbox_{128};
};

}
