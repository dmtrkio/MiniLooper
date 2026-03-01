#include "looper_processor.h"

#include <algorithm>
#include <cassert>
#include <format>
#include <iostream>

#include "audio/audio_engine.h"

namespace looper {

    const char* stateToStr(State state)
    {
        if (state == State::CLEARED) return "CLEARED";
        if (state == State::RECORDING) return "RECORDING";
        if (state == State::PLAYBACK) return "PLAYBACK";
        if (state == State::PAUSED) return "PAUSED";
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

        for (int i = 0; i < tracks_.size(); ++i) {
            tracks_[i].init(i, nChannels, mFrames);
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
        if (!isTrackIndexValid(trackIndex)) return true;
        return tracks_[trackIndex].isEmpty();
    }

    void LooperProcessor::startRecording(int trackIndex) noexcept
    {
        if (!isTrackIndexValid(trackIndex)) return;

        auto& track = tracks_[trackIndex];

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
                const auto framesToLoop = track.nFrames - track.position;
                track.scheduleTransition(State::RECORDING, framesToLoop);
                break;

                //track.state = State::RECORDING;
                //break;
            }
            default:;
        }
    }

    void LooperProcessor::stopRecording(int trackIndex) noexcept
    {
        if (!isTrackIndexValid(trackIndex)) return;

        auto &track = tracks_[trackIndex];

        switch (track.state) {
            case State::RECORDING: {
                if (track.isEmpty()) {
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
                    const auto framesToLoop = track.nFrames - track.position;
                    track.scheduleTransition(State::PLAYBACK, framesToLoop);

                    //track.state = State::PLAYBACK;
                }

                break;
            }
            default:;
        }
    }

    void LooperProcessor::clear(int trackIndex) noexcept
    {
        if (!isTrackIndexValid(trackIndex)) return;

        {
            auto& track = tracks_[trackIndex];
            if (track.state == State::CLEARED) return;

            const auto toErase = std::max(track.position, track.nFrames);
            for (auto& buffer : track.buffers)
                std::ranges::fill_n(buffer.begin(), toErase, 0.0f);

            track.hasPendingTransition = false;
            track.state = State::CLEARED;
            track.position = 0;
            track.nFrames = 0;
        }

        if (std::ranges::all_of(tracks_, [](const auto& track) { return track.isEmpty(); })) {
            transport_.reset(maxFrames_);
        }
    }

    void LooperProcessor::pause(int trackIndex) noexcept
    {
        if (!isTrackIndexValid(trackIndex)) return;

        auto &track = tracks_[trackIndex];

        if (track.state != State::PLAYBACK) return;

        const auto toWait = track.nFrames - track.position;
        track.scheduleTransition(State::PAUSED, toWait);
    }

    void LooperProcessor::resume(int trackIndex) noexcept
    {
        if (!isTrackIndexValid(trackIndex)) return;

        auto &track = tracks_[trackIndex];

        if (track.state != State::PAUSED) return;

        const auto toWait = track.nFrames - track.position;
        track.scheduleTransition(State::PLAYBACK, toWait);
    }

    void LooperProcessor::clearAll() noexcept
    {
        for (int i = 0; i < getNumLooperTracks(); ++i)
            clear(i);
    }

    unsigned int LooperProcessor::getNextGridDivision(int frameIndex) const noexcept
    {
        int target = static_cast<int>(transport_.largestPossibleLoopLength);
        int distance = target - frameIndex;

        while (target > transport_.barLength) {
            const auto newTarget = target / 2;
            const auto newDistance = newTarget - frameIndex;

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
        sharedData_.commandMailbox.consumeAll([&](const LooperCommand& cmd) {
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

        for (auto i{0u}; i < nFrames; ++i) {
            switch (track.state) {
                case State::PLAYBACK: {
                    for (auto ch{0u}; ch < numChannels_; ++ch) {
                        sumBuffers_[ch][i] += track.read(ch);
                    }
                    break;
                }
                case State::RECORDING: {
                    for (auto ch{0u}; ch < numChannels_; ++ch) {
                        const float oldSample = track.read(ch);
                        track.writeAdding(ch, data[ch][i]);
                        sumBuffers_[ch][i] += oldSample;
                    }
                    break;
                }
                default:;
            }

            track.advance(transport_, maxFrames_);
        }
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

        if (barLength == 0) {
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

    void LooperProcessor::Track::init(int index, unsigned int nChannels, unsigned int maxFrames) noexcept
    {
        trackIndex = index;
        buffers.resize(nChannels);
        for (auto& buffer : buffers) {
            buffer.resize(maxFrames);
        }

        hasPendingTransition = false;

        constexpr float crossfadeMs = 10.0f;
        const auto sampleRate = static_cast<float>(audio::AudioEngine::getInstance().getSampleRate());
        crossfadeLength = static_cast<unsigned int>(crossfadeMs * sampleRate / 1000.0f);
    }

    bool LooperProcessor::Track::isEmpty() const noexcept
    {
        return nFrames == 0;
    }

    void LooperProcessor::Track::scheduleTransition(State next, unsigned int when)
    {
        if (when == 0) {
            transitionState(next);
            return;
        }

        pendingState = next;
        hasPendingTransition = true;
        framesToTransition = static_cast<int>(when);
    }

    void LooperProcessor::Track::transitionState(State newState) noexcept
    {
        switch (newState) {
            case State::RECORDING: {
                position = 0;
                state = State::RECORDING;
                break;
            }
            case State::PLAYBACK: {
                if (isEmpty() && state == State::RECORDING) {
                    nFrames = position;
                    position = 0;
                }

                state = State::PLAYBACK;
                break;
            }
            case State::PAUSED: {
                state = State::PAUSED;
                break;
            }
            default:;
        }
    }

    void LooperProcessor::Track::advance(Transport& transport, unsigned int maxFrames) noexcept
    {
        const auto wrapAround = isEmpty() ? (transport.largestPossibleLoopLength + 1) : nFrames;

        if (state != State::CLEARED) {
            position++;

            if (position >= wrapAround) {
                position = 0;
                if (isEmpty() && state == State::RECORDING) {
                    nFrames = wrapAround;
                    if (!transport.isTempoSet()) {
                        transport.setBarLength(nFrames, maxFrames);
                    }
                }
            }
        }

        if (hasPendingTransition) {
            framesToTransition--;
            if (framesToTransition <= 0) {
                hasPendingTransition = false;
                transitionState(pendingState);
            }
        }
    }

    float LooperProcessor::Track::read(unsigned int channel) const noexcept
    {
        if (isEmpty()) return 0.0f;

#define USE_CROSSFADE 0
#if USE_CROSSFADE
        if ((nFrames < crossfadeLength) || (position < (nFrames - crossfadeLength)))
            return buffers[channel][position];

        const auto t = static_cast<float>(position - (nFrames - crossfadeLength)) / static_cast<float>(crossfadeLength - 1);
        const auto fadeIn = t;
        const auto fadeOut = 1.0f - t;

        const auto startSample = buffers[channel][position - nFrames + crossfadeLength] * fadeIn;
        const auto endSample = buffers[channel][position] * fadeOut;

        return startSample + endSample;
#else
        return buffers[channel][position];
#endif
    }

    void LooperProcessor::Track::writeAdding(unsigned int channel, float value) noexcept
    {
        buffers[channel][position] += value;
    }

    void LooperProcessor::Track::overwrite(unsigned int channel, float value) noexcept
    {
        buffers[channel][position] = value;
    }
}
