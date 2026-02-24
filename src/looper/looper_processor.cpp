#include "looper_processor.h"

#include <algorithm>
#include <format>
#include <cassert>

#include "../audio/audio_engine.h"

namespace looper {

    const char* stateToStr(State state)
    {
        if (state == State::CLEARED) return "CLEARED";
        if (state == State::RECORDING) return "RECORDING";
        if (state == State::PLAYBACK) return "PLAYBACK";
        return "Invalid State";
    }

    std::string TrackStateSnapshot::toString() const
    {
        return std::format("nFrames: {}, position: {}, state: {}", nFrames, position, stateToStr(state));
    }

    void LooperProcessor::process(float *const *data, unsigned int nFrames) noexcept
    {
        consumeCommands();

        assert(nFrames <= maxFrames_);
        if (!data || numChannels_ == 0) return;

        processInternal(data, nFrames);
        updateSnapshot();
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
        updateSnapshot();
    }

    void LooperProcessor::onStop()
    {
        clearAll();
    }

    LooperSharedData& LooperProcessor::getSharedData() noexcept
    {
        return sharedData_;
    }

    State LooperProcessor::getState(int trackIndex) const noexcept
    {
        if (!isTrackIndexValid(trackIndex)) return State::CLEARED;
        return tracks_[trackIndex].state;
    }

    unsigned int LooperProcessor::getCurrentPosition(int trackIndex) const noexcept
    {
        if (!isTrackIndexValid(trackIndex)) return 0;
        return tracks_[trackIndex].position;
    }

    unsigned int LooperProcessor::getCurrentNumFrames(int trackIndex) const noexcept
    {
        if (!isTrackIndexValid(trackIndex)) return 0;
        return tracks_[trackIndex].nFrames;
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

        auto& track = tracks_[trackIndex];
        track.transitionTimer.reset();

        if (isAnyTrackCurrentlyRecording()) return;

        switch (track.state) {
            case State::CLEARED: {
                if (!transport_.isTempoSet()) {
                    track.position = 0;
                    track.state = State::RECORDING;
                } else {
                    const auto framesToBar = transport_.barLength - transport_.currentFrame % transport_.barLength;
                    track.scheduleTransition(State::RECORDING, framesToBar);
                }
                break;
            }
            case State::PAUSED: {
                [[fallthrough]];
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
        track.transitionTimer.reset();

        switch (track.state) {
            case State::RECORDING: {
                if (track.nFrames == 0) {
                    if (!transport_.isTempoSet()) {
                        transport_.setBarLength(track.position, maxFrames_);
                        track.nFrames = track.position;
                        track.position = 0;
                        track.state = State::PLAYBACK;
                    } else {
                        const auto toWait = getNextGridDivision(static_cast<int>(track.position)) - track.position;
                        track.scheduleTransition(State::PLAYBACK, toWait);
                    }
                } else {
                    track.state = State::PLAYBACK;
                }

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

        const auto toErase = std::max(position, nFrames);
        for (auto& buffer : buffers)
            std::ranges::fill_n(buffer.begin(), toErase, 0.0f);

        timer.hasNext = false;
        state = State::CLEARED;
        position = 0;
        nFrames = 0;

        if (std::ranges::all_of(tracks_, [](const auto& track) { return track.nFrames == 0; })) {
            transport_.reset(maxFrames_);
        }
    }

    void LooperProcessor::pause(int trackIndex) noexcept
    {
        if (!isTrackIndexValid(trackIndex)) return;

        auto &track = tracks_[trackIndex];

        if (track.state != State::PLAYBACK) return;
        track.transitionTimer.reset();

        const auto toWait = track.nFrames - track.position;
        track.scheduleTransition(State::PAUSED, toWait);
    }

    void LooperProcessor::resume(int trackIndex) noexcept
    {
        if (!isTrackIndexValid(trackIndex)) return;

        auto &track = tracks_[trackIndex];

        if (track.state != State::PAUSED) return;
        track.transitionTimer.reset();

        const auto toWait = track.nFrames - track.position;
        track.scheduleTransition(State::PLAYBACK, toWait);
    }

    void LooperProcessor::clearAll() noexcept
    {
        transport_.reset(maxFrames_);
        for (int i = 0; i < getNumLooperTracks(); ++i)
            clear(i);
    }

    unsigned int LooperProcessor::getNextGridDivision(int frameIndex) const noexcept
    {
        int target = static_cast<int>(transport_.largestPossibleLoopLength);
        int distance = target - frameIndex;

        while (target > transport_.barLength) {
            int newTarget = target / 2;
            int newDistance = newTarget - frameIndex;

            if ((newDistance < 0) || (newDistance > distance)) break;

            target = newTarget;
            distance = newDistance;
        }

        //std::cout << "bars: " << target / transport_.barLength << std::endl;
        return target;
    }

    bool LooperProcessor::isTrackIndexValid(int trackIndex) const noexcept
    {
        return trackIndex >= 0 && trackIndex < getNumLooperTracks();
    }

    bool LooperProcessor::isAnyTrackCurrentlyRecording() const noexcept
    {
        return std::ranges::any_of(tracks_, [](const auto& track) {
            return track.state == State::RECORDING;
        });
    }

    void LooperProcessor::updateSnapshot() noexcept
    {
        auto writer = sharedData_.state.getWriter();
        auto& snapshot = writer.data();

        for (auto i{0u}; i < getNumLooperTracks(); ++i) {
            const auto& track = tracks_[i];
            auto& [nFrames, position, state] = snapshot.tracks[i];
            nFrames = track.nFrames;
            position = track.position;
            state = track.state;
        }
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

        const auto wrapAround = track.nFrames > 0 ? track.nFrames : (transport_.largestPossibleLoopLength + 1);

        for (auto i{0u}; i < nFrames; ++i) {
            track.tick();

            switch (track.state) {
                case State::PLAYBACK: {
                    for (auto ch{0u}; ch < numChannels_; ++ch) {
                        sumBuffers_[ch][i] += track.buffers[ch][track.position];
                    }
                    break;
                }
                case State::RECORDING: {
                    for (auto ch{0u}; ch < numChannels_; ++ch) {
                        const float oldSample = track.buffers[ch][track.position];
                        track.buffers[ch][track.position] += data[ch][i];
                        sumBuffers_[ch][i] += oldSample;
                    }
                    break;
                }
                case State::CLEARED: {
                    continue;
                }
                default:;
            }

            track.position++;
            if (track.position >= wrapAround) {
                track.position = 0;
                if (track.nFrames == 0) {
                    if (track.state == State::RECORDING) {
                        track.nFrames = wrapAround;
                        if (!transport_.isTempoSet()) {
                            transport_.setBarLength(track.nFrames, maxFrames_);
                        }
                    }
                }
            }
        }
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
            switch (nextState) {
                case State::RECORDING: {
                    position = 0;
                    state = State::RECORDING;
                    break;
                }
                case State::PLAYBACK: {
                    if (state == State::RECORDING) {
                        nFrames = position;
                    }
                    position = 0;
                    state = State::PLAYBACK;
                    break;
                }
                case State::PAUSED: {
                    state = State::PAUSED;
                    break;
                }
                default:;
            }

            hasNext = false;
            return true;
        }

        return false;
    }

    bool LooperProcessor::Transport::isTempoSet() const noexcept
    {
        return barLength != 0;
    }

    void LooperProcessor::Transport::tick(unsigned int numFrames) noexcept
    {
        if (isTempoSet()) currentFrame += numFrames;
    }

    void LooperProcessor::Transport::setBarLength(unsigned int nFrames, unsigned int maxFrames) noexcept
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

    void LooperProcessor::Transport::reset(unsigned int maxFrames) noexcept
    {
        setBarLength(0, maxFrames);
    }

    void LooperProcessor::Track::scheduleTransition(State next, unsigned int when)
    {
        auto& [hasNext, nextState, framesLeft] = transitionTimer;
        hasNext = true;
        nextState = next;
        framesLeft = when;
    }

}