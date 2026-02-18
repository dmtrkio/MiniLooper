#include "looper_processor.h"

#include <algorithm>
#include <iostream>

#include "../audio/audio_engine.h"

using namespace looper;

void LooperProcessor::process(float *const *data, unsigned int nFrames) noexcept
{
    consumeCommands();

    if (!data || numChannels_ == 0) return;

    processInternal(data, nFrames);
}

void LooperProcessor::onStart()
{
    const auto& engine = audio::AudioEngine::getInstance();
    const auto nChannels = engine.getNumOutputChannels();
    const auto mFrames = engine.getSampleRate() * MAX_LOOP_LENGTH_IN_SECONDS;

    numChannels_ = nChannels;
    maxFrames_ = mFrames;

    for (auto& track : tracks_) {
        track.buffers.resize(nChannels);
        for (auto& buffer : track.buffers) {
            buffer.resize(mFrames);
        }
    }

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
    return NUM_LOOPER_TRACKS;
}

LooperProcessor::State LooperProcessor::getState(int trackIndex) const noexcept
{
    if (!isTrackIndexValid(trackIndex)) return LooperProcessor::State::CLEARED;
    return tracks_[trackIndex].state.load();
}

unsigned int LooperProcessor::getCurrentPosition(int trackIndex) const noexcept
{
    if (!isTrackIndexValid(trackIndex)) return 0;
    return tracks_[trackIndex].position.load();
}

unsigned int LooperProcessor::getCurrentNumFrames(int trackIndex) const noexcept
{
    if (!isTrackIndexValid(trackIndex)) return 0;
    return tracks_[trackIndex].nFrames.load();
}

bool LooperProcessor::isEmpty(int trackIndex) const noexcept
{
    return getCurrentNumFrames(trackIndex) == 0;
}

LooperMailbox& LooperProcessor::getCommandMailbox() noexcept
{
    return commandMailbox_;
}

void LooperProcessor::startRecording(int trackIndex) noexcept
{
    if (!isTrackIndexValid(trackIndex)) return;

    auto& [state, position, nFrames, buffers] = tracks_[trackIndex];

    switch (state.load()) {
        case State::CLEARED: {
            position.store(0);
            state = State::RECORDING;
            break;
        }
        case State::RECORDING: {
            break;
        }
        case State::PLAYBACK: {
            state = State::RECORDING;
            break;
        }
        default:;
    }
}

void LooperProcessor::stopRecording(int trackIndex) noexcept
{
    if (!isTrackIndexValid(trackIndex)) return;

    auto& [state, position, nFrames, buffers] = tracks_[trackIndex];

    switch (state.load()) {
        case State::CLEARED: {
            break;
        }
        case State::RECORDING: {
            if (isEmpty(0)) {
                nFrames.store(position.load());
                position.store(0);
            }
            state = State::PLAYBACK;
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

    auto& [state, position, nFrames, buffers] = tracks_[trackIndex];
    if (state == State::CLEARED) return;

    stopRecording(trackIndex);

    const auto toErase = nFrames.load();
    for (auto& buffer : buffers)
        std::ranges::fill_n(buffer.begin(), toErase, 0.0f);

    state = State::CLEARED;
    position.store(0);
    nFrames.store(0);
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
    for (int i = 0; i < getNumLooperTracks(); ++i) {
        processTrack(i, data, nFrames);
    }
}

void LooperProcessor::processTrack(int trackIndex, float *const *data, unsigned int nFrames) noexcept
{
    if (!isTrackIndexValid(trackIndex)) return;

    auto& [trackState, position, loopFrames, buffers] = tracks_[trackIndex];

    const auto state = trackState.load();
    if (state == State::CLEARED) return;

    const auto currentNumFrames = loopFrames.load();
    const auto wrapAround = currentNumFrames > 0 ? currentNumFrames : maxFrames_;
    unsigned int pos = position.load();

    if (state == State::PLAYBACK) {
        for (auto i{0u}; i < nFrames; ++i) {
            for (auto ch{0u}; ch < numChannels_; ++ch) {
                data[ch][i] += buffers[ch][pos];
            }

            pos++;
            if (pos >= wrapAround) {
                pos = 0;
                loopFrames.store(wrapAround);
            }
        }
    } else if (state == State::RECORDING) {
        for (auto i{0u}; i < nFrames; ++i) {
            for (auto ch{0u}; ch < numChannels_; ++ch) {
                const float oldSample = buffers[ch][pos];
                buffers[ch][pos] += data[ch][i];
                data[ch][i] += oldSample;
            }

            pos++;
            if (pos >= wrapAround) {
                pos = 0;
                loopFrames.store(wrapAround);
            }
        }
    }

    position.store(pos);
}