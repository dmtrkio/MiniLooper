#include "looper.h"

#include <ranges>
#include <algorithm>

#include "../audio/audio_engine.h"

using namespace looper;

void Looper::process(float **data, unsigned int nFrames) noexcept
{
    consumeCommands();

    if (!data || buffers_.empty()) return;

    processInternal(data, nFrames);
}

void Looper::onStart()
{
    const auto& engine = audio::AudioEngine::getInstance();
    const auto nChannels = engine.getNumOutputChannels();
    const auto mFrames = engine.getSampleRate() * maxLengthInSec_;

    numChannels_ = nChannels;
    maxFrames_ = mFrames;

    buffers_.resize(numChannels_);
    for (auto& b : buffers_)
        b.resize(maxFrames_);

    clear();

    // ensure mailbox is clear from stale messages
    consumeCommands();
}

void Looper::onStop()
{
    clear();
}

LooperMailbox& Looper::getCommandMailbox() noexcept
{
    return commandMailbox_;
}

void Looper::startRecording() noexcept
{
    switch (state_) {
        case State::CLEARED: {
            position_ = 0;
            state_ = State::RECORDING;
            break;
        }
        case State::RECORDING: {
            break;
        }
        case State::PLAYBACK: {
            state_ = State::RECORDING;
            break;
        }
    }
}

void Looper::stopRecording() noexcept
{
    switch (state_) {
        case State::CLEARED: {
            break;
        }
        case State::RECORDING: {
            state_ = State::PLAYBACK;
            if (numFrames_ == 0) {
                numFrames_ = position_;
                position_ = 0;
            }
            break;
        }
        case State::PLAYBACK: {
            break;
        }
    }
}

void Looper::clear() noexcept
{
    for (auto& b : buffers_)
        std::ranges::fill(b, 0.0f);

    state_ = State::CLEARED;
    position_ = 0;
    numFrames_ = 0;
}

void Looper::consumeCommands() noexcept
{
    commandMailbox_.consumeAll([&](const LooperCommand& cmd) {
        cmd.apply(*this);
    });
}

void Looper::processInternal(float **data, unsigned int nFrames) noexcept
{
    if (state_ == State::CLEARED) return;

    const unsigned int wrapAround = numFrames_ > 0 ? numFrames_ : maxFrames_;

    for (auto i{0u}; i < nFrames; ++i) {
        for (auto ch{0u}; ch < numChannels_; ++ch) {
            if (state_ == State::RECORDING) {
                const float oldSample = buffers_[ch][position_];
                buffers_[ch][position_] += data[ch][i];
                data[ch][i] += oldSample;
            } else if (state_ == State::PLAYBACK) {
                data[ch][i] += buffers_[ch][position_];
            }

            position_++;
            if (position_ >= wrapAround) {
                position_ = 0;
                numFrames_ = wrapAround;
            }
        }
    }
}