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
    clearAll();
}

void LooperProcessor::onStop()
{
    clearAll();
}

int LooperProcessor::getNumLooperTracks() const noexcept
{
    return 1;
}

LooperProcessor::State LooperProcessor::getState(int trackIndex) const noexcept
{
    if (!isTrackIndexValid(trackIndex)) return LooperProcessor::State::CLEARED;
    return state_.load();
}

unsigned int LooperProcessor::getCurrentPosition(int trackIndex) const noexcept
{
    if (!isTrackIndexValid(trackIndex)) return 0;
    return position_.load();
}

unsigned int LooperProcessor::getCurrentNumFrames(int trackIndex) const noexcept
{
    if (!isTrackIndexValid(trackIndex)) return 0;
    return numFrames_.load();
}

bool LooperProcessor::isEmpty(int trackIndex) const noexcept
{
    if (!isTrackIndexValid(trackIndex)) return true;
    return getCurrentNumFrames(trackIndex) == 0;
}

LooperMailbox& LooperProcessor::getCommandMailbox() noexcept
{
    return commandMailbox_;
}

void LooperProcessor::startRecording(int trackIndex) noexcept
{
    if (!isTrackIndexValid(trackIndex)) return;

    switch (state_.load()) {
        case State::CLEARED: {
            position_.store(0);
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

void LooperProcessor::stopRecording(int trackIndex) noexcept
{
    if (!isTrackIndexValid(trackIndex)) return;

    switch (state_.load()) {
        case State::CLEARED: {
            break;
        }
        case State::RECORDING: {
            if (isEmpty(0)) {
                numFrames_.store(position_.load());
                position_.store(0);
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

void LooperProcessor::clear(int trackIndex) noexcept
{
    if (!isTrackIndexValid(trackIndex)) return;

    if (state_ == State::CLEARED) return;

    stopRecording(trackIndex);

    const auto toErase = numFrames_.load();
    for (auto& b : buffers_)
        std::ranges::fill_n(b.begin(), toErase, 0.0f);

    state_ = State::CLEARED;
    position_.store(0);
    numFrames_.store(0);
}

void LooperProcessor::clearAll() noexcept
{
    for (int i = 0; i < getNumLooperTracks(); ++i)
        clear(i);
}

const char* LooperProcessor::stateToStr(State state)
{
    if (state == State::CLEARED) return "CLEARED";
    if (state == State::RECORDING) return "RECORDING";
    if (state == State::PLAYBACK) return "PLAYBACK";
    return "Invalid State";
}


bool LooperProcessor::isTrackIndexValid(int trackIndex) const noexcept
{
    return trackIndex >= 0 && trackIndex < getNumLooperTracks();
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

    const auto currentNumFrames = numFrames_.load();
    const auto wrapAround = currentNumFrames > 0 ? currentNumFrames : maxFrames_;
    unsigned int pos = position_.load();

    if (state == State::PLAYBACK) {
        for (auto i{0u}; i < nFrames; ++i) {
            for (auto ch{0u}; ch < numChannels_; ++ch) {
                data[ch][i] += buffers_[ch][pos];
            }

            pos++;
            if (pos >= wrapAround) {
                pos = 0;
                numFrames_.store(wrapAround);
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
                numFrames_.store(wrapAround);
            }
        }
    }

    position_.store(pos);
}