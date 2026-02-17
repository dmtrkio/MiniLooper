#include "looper_processor.h"

#include <algorithm>
#include <iostream>

#include "../audio/audio_engine.h"

using namespace looper;

void LooperProcessor::process(float *const *data, unsigned int nFrames) noexcept
{
    consumeCommands();

    if (!data || buffers_.empty()) return;

    processInternal(data, nFrames);
}

void LooperProcessor::onStart()
{
    const auto& engine = audio::AudioEngine::getInstance();
    const auto nChannels = engine.getNumOutputChannels();
    const auto mFrames = engine.getSampleRate() * MAX_LOOP_LENGTH_IN_SECONDS;

    numChannels_ = nChannels;
    maxFrames_ = mFrames;

    buffers_.resize(numChannels_);
    for (auto& b : buffers_)
        b.resize(maxFrames_);

    // ensure mailbox is clear from stale messages
    consumeCommands();
    clear();
}

void LooperProcessor::onStop()
{
    clear();
}

LooperProcessor::State LooperProcessor::getState() const noexcept
{
    return state_.load();
}

unsigned int LooperProcessor::getCurrentPosition() const noexcept
{
    return position_.load(std::memory_order_relaxed);
}

unsigned int LooperProcessor::getCurrentNumFrames() const noexcept
{
    return numFrames_.load(std::memory_order_relaxed);
}

bool LooperProcessor::isEmpty() const noexcept
{
    return getCurrentNumFrames() == 0;
}

LooperMailbox& LooperProcessor::getCommandMailbox() noexcept
{
    return commandMailbox_;
}

void LooperProcessor::startRecording() noexcept
{
    switch (state_.load()) {
        case State::CLEARED: {
            position_.store(0, std::memory_order_relaxed);
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
        default:;
    }
}

void LooperProcessor::stopRecording() noexcept
{
    switch (state_.load()) {
        case State::CLEARED: {
            break;
        }
        case State::RECORDING: {
            if (isEmpty()) {
                numFrames_.store(position_.load(std::memory_order_relaxed), std::memory_order_relaxed);
                position_.store(0, std::memory_order_relaxed);
            }
            state_ = State::PLAYBACK;
            break;
        }
        case State::PLAYBACK: {
            break;
        }
        default:;
    }
}

void LooperProcessor::clear() noexcept
{
    if (state_ == State::CLEARED) return;

    stopRecording();

    const auto toErase = numFrames_.load(std::memory_order_relaxed);
    for (auto& b : buffers_)
        std::ranges::fill_n(b.begin(), toErase, 0.0f);

    state_ = State::CLEARED;
    position_.store(0, std::memory_order_relaxed);
    numFrames_.store(0, std::memory_order_relaxed);
}

const char* LooperProcessor::stateToStr(State state)
{
    if (state == State::CLEARED) return "CLEARED";
    if (state == State::RECORDING) return "RECORDING";
    if (state == State::PLAYBACK) return "PLAYBACK";
    return "Invalid State";
}

void LooperProcessor::consumeCommands() noexcept
{
    commandMailbox_.consumeAll([&](const LooperCommand& cmd) {
        cmd.apply(*this);
    });
}

void LooperProcessor::processInternal(float *const *data, unsigned int nFrames) noexcept
{
    const auto state = state_.load();

    if (state == State::CLEARED) return;

    const auto currentNumFrames = numFrames_.load(std::memory_order_relaxed);
    const auto wrapAround = currentNumFrames > 0 ? currentNumFrames : maxFrames_;
    unsigned int pos = position_.load(std::memory_order_relaxed);

    if (state == State::PLAYBACK) {
        for (auto i{0u}; i < nFrames; ++i) {
            for (auto ch{0u}; ch < numChannels_; ++ch) {
                data[ch][i] += buffers_[ch][pos];
            }

            pos++;
            if (pos >= wrapAround) {
                pos = 0;
                numFrames_.store(wrapAround, std::memory_order_relaxed);
            }
        }
    } else if (state == State::RECORDING) {
        for (auto i{0u}; i < nFrames; ++i) {
            for (auto ch{0u}; ch < numChannels_; ++ch) {
                const float oldSample = buffers_[ch][pos];
                buffers_[ch][pos] += data[ch][i];
                data[ch][i] += oldSample;
            }

            pos++;
            if (pos >= wrapAround) {
                pos = 0;
                numFrames_.store(wrapAround, std::memory_order_relaxed);
            }
        }
    }

    position_.store(pos, std::memory_order_relaxed);
}