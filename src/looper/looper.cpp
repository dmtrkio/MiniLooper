#include "looper.h"

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

unsigned int Looper::getCurrentPosition() const noexcept
{
    return position_.load(std::memory_order_acquire);
}

unsigned int Looper::getCurrentNumFrames() const noexcept
{
    return numFrames_.load(std::memory_order_acquire);
}

bool Looper::isEmpty() const noexcept
{
    return getCurrentNumFrames() == 0;
}

LooperMailbox& Looper::getCommandMailbox() noexcept
{
    return commandMailbox_;
}

void Looper::startRecording() noexcept
{
    switch (state_) {
        case State::CLEARED: {
            position_.store(0, std::memory_order_release);
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
            if (isEmpty()) {
                numFrames_.store(position_.load(std::memory_order_acquire), std::memory_order_release);
                position_.store(0, std::memory_order_release);
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
    position_.store(0, std::memory_order_release);
    numFrames_.store(0, std::memory_order_release);
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

    const auto currentNumFrames = numFrames_.load(std::memory_order_acquire);
    const auto wrapAround = currentNumFrames > 0 ? currentNumFrames : maxFrames_;
    unsigned int pos = position_.load(std::memory_order_acquire);

    for (auto i{0u}; i < nFrames; ++i) {
        for (auto ch{0u}; ch < numChannels_; ++ch) {
            if (state_ == State::RECORDING) {
                const float oldSample = buffers_[ch][pos];
                buffers_[ch][pos] += data[ch][i];
                data[ch][i] += oldSample;
            } else if (state_ == State::PLAYBACK) {
                data[ch][i] += buffers_[ch][pos];
            }

            pos++;
            if (pos >= wrapAround) {
                pos = 0;
                numFrames_ = wrapAround;
            }
        }
    }

    position_.store(pos, std::memory_order_release);
}