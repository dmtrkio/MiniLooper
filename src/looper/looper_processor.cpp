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

        auto& track = tracks_[trackIndex];
        track.transitionTimer.reset();

        if (isAnyTrackCurrentlyRecording()) return;

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

        switch (track.state.load()) {
            case State::RECORDING: {
                if (track.nFrames == 0) {
                    const auto nFrames = track.position.load();
                    if (!transport_.isTempoSet()) {
                        transport_.setBarLength(nFrames, maxFrames_);
                        track.nFrames.store(nFrames);
                        track.position.store(0);
                        track.state = State::PLAYBACK;
                    } else {
                        const auto toWait = getNextGridDivision(static_cast<int>(nFrames)) - nFrames;
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

    void LooperProcessor::pause(int trackIndex) noexcept
    {
        if (!isTrackIndexValid(trackIndex)) return;

        auto &track = tracks_[trackIndex];

        if (track.state != State::PLAYBACK) return;
        track.transitionTimer.reset();

        const auto toWait = track.nFrames.load() - track.position.load();
        track.scheduleTransition(State::PAUSED, toWait);
    }

    void LooperProcessor::resume(int trackIndex) noexcept
    {
        if (!isTrackIndexValid(trackIndex)) return;

        auto &track = tracks_[trackIndex];

        if (track.state != State::PAUSED) return;
        track.transitionTimer.reset();

        const auto toWait = track.nFrames.load() - track.position.load();
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
            return track.state.load() == State::RECORDING;
        });
    }

    void LooperProcessor::updateSnapshot() noexcept
    {
        auto writer = sharedData_.state.getWriter();
        auto& snapshot = writer.data();

        for (auto i{0u}; i < getNumLooperTracks(); ++i) {
            const auto& track = tracks_[i];
            auto& [nFrames, position, state] = snapshot.tracks[i];
            nFrames = track.nFrames.load();
            position = track.position.load();
            state = track.state.load();
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

        auto state = track.state.load();

        auto currentNumFrames = track.nFrames.load();
        const auto wrapAround = currentNumFrames > 0 ? currentNumFrames : (transport_.largestPossibleLoopLength + 1);
        unsigned int pos = track.position.load();

        for (auto i{0u}; i < nFrames; ++i) {
            if (track.tick()) {
                state = track.state.load();
                currentNumFrames = track.nFrames.load();
                pos = track.position.load();
            }

            switch (state) {
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
                default:;
            }

            pos++;
            if (pos >= wrapAround) {
                pos = 0;
                if (currentNumFrames == 0) {
                    if (state == State::RECORDING) {
                        currentNumFrames = wrapAround;
                        if (!transport_.isTempoSet()) {
                            transport_.setBarLength(currentNumFrames, maxFrames_);
                        }
                    }
                }
            }
        }

        if (state != State::CLEARED) {
            track.nFrames.store(currentNumFrames);
            track.position.store(pos);
        } else {
            track.nFrames.store(0);
            track.position.store(0);
        }

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
            switch (nextState) {
                case State::RECORDING: {
                    position.store(0);
                    state.store(State::RECORDING);
                    break;
                }
                case State::PLAYBACK: {
                    if (state == State::RECORDING) {
                        nFrames.store(position.load());
                    }
                    position.store(0);
                    state.store(State::PLAYBACK);
                    break;
                }
                case State::PAUSED: {
                    state.store(State::PAUSED);
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