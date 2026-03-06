#include "looper_processor.h"

#include <algorithm>
#include <cassert>

#include "audio/audio_engine.h"

namespace looper {
    const char* stateToStr(State state)
    {
        if (state == State::Cleared) return "Cleared";
        if (state == State::Recording) return "Recording";
        if (state == State::Playback) return "Playback";
        if (state == State::Paused) return "Paused";
        return "Unknown State";
    }

    void LooperProcessor::process(float *const *data, unsigned int nFrames) noexcept
    {
        assert(nFrames <= maxFrames_);
        if (!data || numChannels_ == 0) return;

        processInternal(data, nFrames);
    }

    void LooperProcessor::onStart()
    {
        const auto& engine = audio::AudioEngine::getInstance();
        const auto nChannels = engine.getNumOutputChannels();
        assert(nChannels == 2);
        const auto mFrames = engine.getSampleRate() * kMaxLoopSecs;

        numChannels_ = nChannels;
        maxFrames_ = mFrames;

        for (int i = 0; i < tracks_.size(); ++i) {
            tracks_[i].init(i, nChannels, mFrames);
        }

        mixer_.prepare(tracks_.size());

        sumBuffers_.resize(nChannels);
        for (auto& buffer : sumBuffers_) {
            buffer.resize(mFrames);
        }

        transport_.reset(maxFrames_);
    }

    void LooperProcessor::onStop()
    {
        clearAll();
    }

    Mixer& LooperProcessor::getMixer() noexcept
    {
        return mixer_;
    }

    State LooperProcessor::getState(int trackIndex) const noexcept
    {
        if (!isTrackIndexValid(trackIndex)) return State::Cleared;
        return tracks_[trackIndex].state;
    }

    unsigned int LooperProcessor::getCurrentPosition(int trackIndex) const noexcept
    {
        if (!isTrackIndexValid(trackIndex)) return 0;
        const auto &track = tracks_[trackIndex];
        return track.phase(transport_.currentFrame);
    }

    unsigned int LooperProcessor::getCurrentNumFrames(int trackIndex) const noexcept
    {
        if (!isTrackIndexValid(trackIndex)) return 0;
        return tracks_[trackIndex].length;
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
            case State::Cleared: {
                if (!transport_.isTempoSet()) {
                    track.start = transport_.currentFrame;
                    track.state = State::Recording;
                } else {
                    const auto framesToBar = transport_.barLength - transport_.currentFrame % transport_.barLength;
                    track.scheduleTransition(State::Recording, framesToBar);
                }
                break;
            }
            case State::Paused: {
                [[fallthrough]];
            }
            case State::Playback: {
                const auto framesToLoop = track.length - track.phase(transport_.currentFrame);
                track.scheduleTransition(State::Recording, framesToLoop);
                break;
            }
            default:;
        }
    }

    void LooperProcessor::stopRecording(int trackIndex) noexcept
    {
        if (!isTrackIndexValid(trackIndex)) return;

        auto &track = tracks_[trackIndex];

        switch (track.state) {
            case State::Recording: {
                if (track.isEmpty()) {
                    const auto length = transport_.currentFrame - track.start;
                    if (!transport_.isTempoSet()) {
                        transport_.setBarLength(length, maxFrames_);
                        track.start = 0u;
                        track.length = length;
                        track.state = State::Playback;
                    } else {
                        const auto toWait = getNextGridDivision(static_cast<int>(length)) - length;
                        track.scheduleTransition(State::Playback, toWait);
                    }
                } else {
                    const auto framesToLoop = track.length - track.phase(transport_.currentFrame);
                    track.scheduleTransition(State::Playback, framesToLoop);
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
            if (track.state == State::Cleared) return;

            const auto toErase = std::min(std::max<unsigned int>(
                transport_.currentFrame - track.start,
                track.length
            ), maxFrames_);

            for (auto& buffer : track.buffers)
                std::ranges::fill_n(buffer.begin(), toErase, 0.0f);

            track.hasPendingTransition = false;
            track.state = State::Cleared;
            track.start = 0u;
            track.length = 0u;
        }

        if (std::ranges::all_of(tracks_, [](const auto& track) { return track.isEmpty(); })) {
            transport_.reset(maxFrames_);
        }
    }

    void LooperProcessor::pause(int trackIndex) noexcept
    {
        if (!isTrackIndexValid(trackIndex)) return;

        auto &track = tracks_[trackIndex];

        if (track.state != State::Playback) return;

        const auto toWait = track.length - track.phase(transport_.currentFrame);
        track.scheduleTransition(State::Paused, toWait);
    }

    void LooperProcessor::resume(int trackIndex) noexcept
    {
        if (!isTrackIndexValid(trackIndex)) return;

        auto &track = tracks_[trackIndex];

        if (track.state != State::Paused) return;

        const auto toWait = track.length - track.phase(transport_.currentFrame);
        track.scheduleTransition(State::Playback, toWait);
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

        return target;
    }

    constexpr bool LooperProcessor::isTrackIndexValid(int trackIndex)
    {
        return trackIndex >= 0 && trackIndex < getNumLooperTracks();
    }

    bool LooperProcessor::isAnyTrackCurrentlyRecording() const noexcept
    {
        return std::ranges::any_of(tracks_, [](const auto& track) {
            return (track.state == State::Recording) || (track.hasPendingTransition && track.pendingState == State::Recording);
        });
    }

    void LooperProcessor::processInternal(float *const *data, unsigned int nFrames) noexcept
    {
        for (auto& buffer : sumBuffers_) {
            std::ranges::fill_n(buffer.begin(), nFrames, 0.0f);
        }

        for (int i = 0; i < getNumLooperTracks(); ++i) {
            processTrack(i, data, nFrames);
        }

        mixer_.process(data, nFrames);

        transport_.tick(nFrames);
    }

    void LooperProcessor::processTrack(const int trackIndex, float *const *data, const unsigned int nFrames) noexcept
    {
        if (!isTrackIndexValid(trackIndex)) return;

        auto &track = tracks_[trackIndex];
        auto &loopBufferL = track.buffers[0];
        auto &loopBufferR = track.buffers[1];
        auto [mixerL, mixerR] = mixer_.getChannelBuffers(trackIndex);

        for (auto i{0u}; i < nFrames; ++i) {
            const auto pos = track.phase(transport_.currentFrame + i);

            if (track.hasPendingTransition) {
                track.framesToTransition--;
                if (track.framesToTransition <= 0) {
                    track.hasPendingTransition = false;
                    track.transitionState(track.pendingState, transport_.currentFrame + i);
                }
            }

            const auto [fadeIn, fadeOut] = track.getFadeScalars(pos);
            const auto fade = fadeIn * fadeOut;

            switch (track.state) {
                case State::Playback: {
                    mixerL[i] = loopBufferL[pos] * fade;
                    mixerR[i] = loopBufferR[pos] * fade;
                    break;
                }
                case State::Recording: {
                    mixerL[i] = loopBufferL[pos] * fade;
                    mixerR[i] = loopBufferR[pos] * fade;
                    loopBufferL[pos] += data[0][i];
                    loopBufferR[pos] += data[1][i];
                    break;
                }
                default: {
                    mixerL[i] = 0.0f;
                    mixerR[i] = 0.0f;
                    break;
                }
            }

            if (pos == (transport_.largestPossibleLoopLength - 1)) {
                if (track.isEmpty() && track.state == State::Recording) {
                    track.length = pos + 1;
                }
                if (!transport_.isTempoSet()) {
                    transport_.setBarLength(track.length, maxFrames_);
                }
            }
        }
    }

    bool LooperProcessor::Transport::isTempoSet() const noexcept
    {
        return barLength != 0;
    }

    void LooperProcessor::Transport::tick(const unsigned int nFrames) noexcept
    {
        currentFrame = (currentFrame + nFrames);
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

        for (int i = kGridMultipliers.size() - 1; i >= 0; --i) {
            const auto target = static_cast<unsigned int>(kGridMultipliers[i] * static_cast<float>(barLength));
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

        start = 0u;
        length = 0u;

        hasPendingTransition = false;

        const auto sampleRate = static_cast<float>(audio::AudioEngine::getInstance().getSampleRate());
        fadeLength = static_cast<unsigned int>(kFadeLengthMs * sampleRate / 1000.0f);
    }

    bool LooperProcessor::Track::isEmpty() const noexcept
    {
        return length == 0;
    }

    void LooperProcessor::Track::scheduleTransition(State next, unsigned int when)
    {
        pendingState = next;
        hasPendingTransition = true;
        framesToTransition = static_cast<int>(when);
    }

    void LooperProcessor::Track::transitionState(State newState, const unsigned int transportFrame) noexcept
    {
        switch (newState) {
            case State::Recording: {
                start = transportFrame;
                state = State::Recording;
                break;
            }
            case State::Playback: {
                if (isEmpty() && state == State::Recording) {
                    length = transportFrame - start;
                }

                state = State::Playback;
                break;
            }
            case State::Paused: {
                state = State::Paused;
                break;
            }
            default:;
        }
    }

    unsigned int LooperProcessor::Track::phase(const unsigned int transportFrame) const noexcept
    {
        if (state == State::Recording && length == 0)
            return transportFrame - start;

        if (length == 0)
            return 0u;

        return (transportFrame - start) % length;
    }

    std::tuple<float, float> LooperProcessor::Track::getFadeScalars(const unsigned int pos) const noexcept
    {
        float fadeIn = 1.0f;
        float fadeOut = 1.0f;

        if (pos <= fadeLength) {
            fadeIn = static_cast<float>(pos) / static_cast<float>(fadeLength + 1);
        }

        if ((length - pos) <= fadeLength) {
            fadeOut = static_cast<float>(length - pos) / static_cast<float>(fadeLength + 1);
        }

        return std::make_tuple(fadeIn, fadeOut);
    }
}