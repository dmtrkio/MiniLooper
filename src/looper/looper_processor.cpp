#include "looper_processor.h"

#include <algorithm>
#include <iostream>
#include <cassert>

#include "../audio/audio_engine.h"

using namespace looper;

void LooperProcessor::process(float *const *data, unsigned int nFrames) noexcept
{
    consumeCommands();

    assert(nFrames <= maxFrames_);
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
        track.init(nChannels, mFrames);
    }

    sumBuffers_.resize(nChannels);
    for (auto& buffer : sumBuffers_) {
        buffer.resize(mFrames);
    }

    transport_.reset(maxFrames_);

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
    if (!isTrackIndexValid(trackIndex)) return State::CLEARED;
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
    if (isAnyTrackCurrentlyRecording()) return;

    auto& track = tracks_[trackIndex];

    switch (track.state.load()) {
        case State::CLEARED: {
            if (!transport_.isTempoSet()) {
                track.position.store(0);
                track.state = State::RECORDING;
            } else {
                const auto framesToBar = transport_.barLength - transport_.currentFrame % transport_.barLength;
                track.scheduleTransition(State::RECORDING, framesToBar);
            }
            break;
        }
        case State::RECORDING: {
            break;
        }
        case State::PLAYBACK: {
            track.state = State::RECORDING;
            break;
        }
        default:;
    }
}

void LooperProcessor::stopRecording(int trackIndex) noexcept
{
    if (!isTrackIndexValid(trackIndex)) return;

    auto &track = tracks_[trackIndex];

    switch (track.state.load()) {
        case State::CLEARED: {
            break;
        }
        case State::RECORDING: {
            if (track.nFrames == 0) {
                const auto nFrames = track.position.load();
                if (!transport_.isTempoSet()) {
                    transport_.setBarLength(nFrames, maxFrames_);
                    track.nFrames.store(nFrames);
                    track.position.store(0);
                    track.state = State::PLAYBACK;
                } else {
                    const auto toWait = gridSnap(nFrames);
                    std::cout << "bar length: " << transport_.barLength << std::endl;
                    std::cout << "frames: " << (toWait + nFrames) << std::endl;
                    track.scheduleTransition(State::PLAYBACK, toWait);
                }
            } else {
                track.state = State::PLAYBACK;
            }

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

    auto& [state, position, nFrames, buffers, timer] = tracks_[trackIndex];
    if (state == State::CLEARED) return;

    const auto toErase = std::max(position.load(), nFrames.load());
    for (auto& buffer : buffers)
        std::ranges::fill_n(buffer.begin(), toErase, 0.0f);

    timer.hasNext = false;
    state = State::CLEARED;
    position.store(0);
    nFrames.store(0);

    if (std::ranges::all_of(tracks_, [](const auto& track) { return track.nFrames.load() == 0; })) {
        transport_.reset(maxFrames_);
    }
}

void LooperProcessor::clearAll() noexcept
{
    transport_.reset(maxFrames_);
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

unsigned int LooperProcessor::gridSnap(unsigned int frameIndex) const noexcept
{
    if (!transport_.isTempoSet()) return 0;

    float best = INFINITY;
    const auto limit = static_cast<float>(transport_.largestPossibleLoopLength);

    for (const auto mul : GRID_MULTIPLIERS) {
        const auto target = static_cast<float>(transport_.barLength) * mul;
        if (target >= limit) break;

        const auto gap = target - static_cast<float>(frameIndex);
        if (gap < 0) break;
        best = std::min(best, gap);
    }

    return static_cast<unsigned int>(best);
}

bool LooperProcessor::isTrackIndexValid(int trackIndex) const noexcept
{
    return trackIndex >= 0 && trackIndex < getNumLooperTracks();
}

bool LooperProcessor::isAnyTrackCurrentlyRecording() const noexcept
{
    return std::ranges::any_of(tracks_, [](const auto& track) {
        return track.state.load() == State::RECORDING;
    });
}

void LooperProcessor::consumeCommands() noexcept
{
    commandMailbox_.consumeAll([&](const LooperCommand& cmd) {
        cmd.apply(*this);
    });
}

void LooperProcessor::processInternal(float *const *data, unsigned int nFrames) noexcept
{
    for (auto& buffer : sumBuffers_) {
        std::ranges::fill_n(buffer.begin(), nFrames, 0.0f);
    }

    transport_.tick(nFrames);

    for (int i = 0; i < getNumLooperTracks(); ++i) {
        processTrack(i, data, nFrames);
    }

    for (auto ch{0u}; ch < numChannels_; ++ch) {
        const auto& buffer = sumBuffers_[ch];
        for (auto i{0u}; i < nFrames; ++i) {
            data[ch][i] += buffer[i];
        }
    }
}

void LooperProcessor::processTrack(int trackIndex, float *const *data, unsigned int nFrames) noexcept
{
    if (!isTrackIndexValid(trackIndex)) return;

    auto &track = tracks_[trackIndex];

    auto state = track.state.load();

    auto currentNumFrames = track.nFrames.load();
    const auto wrapAround = currentNumFrames > 0 ? currentNumFrames : transport_.largestPossibleLoopLength;
    unsigned int pos = track.position.load();

    for (auto i{0u}; i < nFrames; ++i) {
        if (track.tick()) state = track.state.load();

        switch (state) {
            case State::CLEARED: {
                continue;
            }
            case State::PLAYBACK: {
                for (auto ch{0u}; ch < numChannels_; ++ch) {
                    sumBuffers_[ch][i] += track.buffers[ch][pos];
                }
                break;
            }
            case State::RECORDING: {
                for (auto ch{0u}; ch < numChannels_; ++ch) {
                    const float oldSample = track.buffers[ch][pos];
                    track.buffers[ch][pos] += data[ch][i];
                    sumBuffers_[ch][i] += oldSample;
                }
                break;
            }
        }

        pos++;
        if (pos >= wrapAround) {
            pos = 0;
            if (currentNumFrames == 0) {
                if (state == State::RECORDING) {
                    state = State::PLAYBACK;
                    currentNumFrames = wrapAround;
                }
            }
        }
    }

    track.nFrames.store(currentNumFrames);
    track.position.store(pos);
    track.state.store(state);
}

void LooperProcessor::Track::init(unsigned int nChannels, unsigned int maxFrames) noexcept
{
    buffers.resize(nChannels);
    for (auto& buffer : buffers) {
        buffer.resize(maxFrames);
    }

    transitionTimer.hasNext = false;
    transitionTimer.framesLeft = 0;
}

bool LooperProcessor::Track::tick()
{
    auto& [hasNext, nextState, framesLeft] = transitionTimer;

    if (!hasNext) return false;

    if (framesLeft-- == 0) {
        if (nextState == State::RECORDING) {
            position.store(0);
            state.store(State::RECORDING);
        } else if (nextState == State::PLAYBACK) {
            nFrames.store(position.load());
            position.store(0);
            state.store(State::PLAYBACK);
        }

        std::cout << stateToStr(state.load()) << std::endl;

        hasNext = false;
        return true;
    }

    return false;
}

void LooperProcessor::Track::scheduleTransition(State next, unsigned int when)
{
    auto& [hasNext, nextState, framesLeft] = transitionTimer;
    hasNext = true;
    nextState = next;
    framesLeft = when;
}
