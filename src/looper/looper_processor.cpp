#include "looper_processor.h"

#include <algorithm>
#include <cassert>

namespace ml::looper {
    const char* stateToStr(State state)
    {
        if (state == State::Cleared) return "Cleared";
        if (state == State::Recording) return "Recording";
        if (state == State::Playback) return "Playback";
        if (state == State::Paused) return "Paused";
        return "Unknown State";
    }

    LooperProcessor::LooperProcessor() : mixer_(kLooperTrackCount) {}

    void LooperProcessor::prepare(float sampleRate)
    {
        sampleRate_ = sampleRate;
        maxFrames_ = static_cast<FrameInt>(sampleRate * kMaxLoopSecs);
        assert(maxFrames_ > 0);

        transport_.reset(maxFrames_, sampleRate);

        for (auto& track : tracks_) {
            track.init(&transport_, maxFrames_, sampleRate);
        }

        mixer_.prepare(sampleRate);
    }

    void LooperProcessor::process(float *const *data, FrameInt nFrames) noexcept
    {
        assert(nFrames <= maxFrames_);
        if (!data) return;

        for (int trackIndex = 0; trackIndex < getNumLooperTracks(); ++trackIndex) {
            auto &track = tracks_[trackIndex];
            auto [mixerL, mixerR] = mixer_.getChannelBuffers(trackIndex);
            float* out[2]{mixerL, mixerR};
            track.process(data, out, nFrames);
        }

        mixer_.process(data, nFrames);

        transport_.tick(nFrames);
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

    FrameInt LooperProcessor::getMaxFramesInLoop() const noexcept
    {
        return maxFrames_;
    }

    FrameInt LooperProcessor::getCurrentPosition(int trackIndex) const noexcept
    {
        if (!isTrackIndexValid(trackIndex)) return 0;
        return tracks_[trackIndex].getPosition();
    }

    FrameInt LooperProcessor::getCurrentNumFrames(int trackIndex) const noexcept
    {
        if (!isTrackIndexValid(trackIndex)) return 0;
        return tracks_[trackIndex].getLength();
    }

    bool LooperProcessor::isEmpty(int trackIndex) const noexcept
    {
        if (!isTrackIndexValid(trackIndex)) return true;
        return tracks_[trackIndex].isEmpty();
    }

    std::optional<float> LooperProcessor::getApproxBpm() const noexcept
    {
        return transport_.getApproxBPM();
    }

    void LooperProcessor::startRecording(int trackIndex, bool synced) noexcept
    {
        if (!isTrackIndexValid(trackIndex)) return;
        if (isAnyTrackCurrentlyRecording()) return;
        tracks_[trackIndex].startRecording(synced);
    }

    void LooperProcessor::stopRecording(int trackIndex, bool synced) noexcept
    {
        if (!isTrackIndexValid(trackIndex)) return;
        tracks_[trackIndex].stopRecording(synced);
    }

    void LooperProcessor::clear(int trackIndex) noexcept
    {
        if (!isTrackIndexValid(trackIndex)) return;
        if (!tracks_[trackIndex].clear()) return;
        if (std::ranges::all_of(tracks_, [](const auto& track) { return track.isEmpty(); })) {
            transport_.reset(maxFrames_, sampleRate_);
        }
    }

    void LooperProcessor::pause(int trackIndex, bool synced) noexcept
    {
        if (!isTrackIndexValid(trackIndex)) return;
        tracks_[trackIndex].pause(synced);
    }

    void LooperProcessor::resume(int trackIndex, bool synced) noexcept
    {
        if (!isTrackIndexValid(trackIndex)) return;
        tracks_[trackIndex].resume(synced);
    }

    void LooperProcessor::clearAll() noexcept
    {
        for (auto& track : tracks_) {
            track.clear();
        }
        transport_.reset(maxFrames_, sampleRate_);
    }

    FrameInt LooperProcessor::copyLoop(int trackIndex, float* const* data, const FrameInt capacity) const noexcept
    {
        if (!isTrackIndexValid(trackIndex)) return 0;
        auto &track = tracks_[trackIndex];
        const auto toCopy = std::min(capacity, track.length);
        std::ranges::copy_n(track.buffers[0].begin(), toCopy, data[0]);
        std::ranges::copy_n(track.buffers[1].begin(), toCopy, data[1]);
        return toCopy;
    }

    void LooperProcessor::extractThumbnail(int trackIndex, ThumbnailSnapshot& out) const noexcept
    {
        if (!isTrackIndexValid(trackIndex)) return;
        const auto& track = tracks_[trackIndex];
        out = track.thumbnail;
    }

    bool LooperProcessor::isAnyTrackCurrentlyRecording() const noexcept
    {
        return std::ranges::any_of(tracks_, [](const auto& track) {
            return (track.state == State::Recording) || (track.hasPendingTransition && track.pendingState == State::Recording);
        });
    }

    bool LooperProcessor::Transport::isTempoSet() const noexcept
    {
        return unitLength != 0;
    }

    std::optional<float> LooperProcessor::Transport::getApproxBPM() const noexcept
    {
        if (!isTempoSet()) return std::nullopt;
        return (sampleRate * 60.0f) / static_cast<float>(unitLength);
    }

    void LooperProcessor::Transport::tick(const FrameInt nFrames) noexcept
    {
        currentFrame += nFrames;
    }

    void LooperProcessor::Transport::setTempo(FrameInt firstLoopLength) noexcept
    {
        unitLength = estimateQuarterNoteUnit(firstLoopLength, sampleRate);

        if (unitLength == 0) {
            largestPossibleLoopLength = maxFrames;
            return;
        }

        largestPossibleLoopLength = maxFrames - maxFrames % unitLength;
    }

    void LooperProcessor::Transport::reset(FrameInt maxFrameCount, float sr) noexcept
    {
        sampleRate = sr;
        maxFrames = maxFrameCount;
        currentFrame = 0;
        unitLength = 0;
        largestPossibleLoopLength = maxFrameCount;
    }

    void LooperProcessor::Track::init(Transport* transport, FrameInt maxFrames, float sampleRate) noexcept
    {
        assert(transport);
        transportPtr = transport;

        for (auto& buffer : buffers) {
            buffer.resize(maxFrames);
        }

        start = 0;
        length = 0;

        hasPendingTransition = false;

        fadeLength = static_cast<FrameInt>(kFadeLengthMs * sampleRate / 1000.0f);
    }

    void LooperProcessor::Track::process(const float *const *in, float *const *out, const FrameInt nFrames) noexcept
    {
        const auto [inL, inR] = std::make_pair(in[0], in[1]);
        auto [outL, outR] = std::make_pair(out[0], out[1]);
        auto &loopBufferL = buffers[0];
        auto &loopBufferR = buffers[1];
        auto& transport = *transportPtr;

        for (FrameInt i{}; i < nFrames; ++i) {
            const auto now = transport.currentFrame + i;
            const auto pos = phase(now);

            handlePendingTransition(now);

            const auto [fadeIn, fadeOut] = getFadeScalars(pos);
            const auto fade = fadeIn * fadeOut;

            switch (state) {
                case State::Playback: {
                    outL[i] = loopBufferL[pos] * fade;
                    outR[i] = loopBufferR[pos] * fade;
                    break;
                }
                case State::Recording: {
                    outL[i] = loopBufferL[pos] * fade;
                    outR[i] = loopBufferR[pos] * fade;
                    loopBufferL[pos] += inL[i];
                    loopBufferR[pos] += inR[i];
                    break;
                }
                default: {
                    outL[i] = 0.0f;
                    outR[i] = 0.0f;
                    break;
                }
            }

            if (pos == (transport.largestPossibleLoopLength - 1)) {
                // if clamp the largest possible loop length 
                if (isEmpty() && state == State::Recording) {
                    length = transport.largestPossibleLoopLength;
                    // if no tempo set, set it to the length of the first recorded loop
                    if (!transport.isTempoSet()) {
                        transport.setTempo(length);
                        // only stop recording if it is the first loop, otherwise start overdubbing automatically
                        state = State::Playback;
                    }
                }
            }
        }

        updateThumbnail();
    }

    void LooperProcessor::Track::startRecording(bool synced) noexcept
    {
        auto& transport = *transportPtr;
        switch (state) {
            case State::Cleared: {
                if (!transport.isTempoSet()) {
                    transport.currentFrame = 0;
                    start = 0;
                    state = State::Recording;
                } else {
                    const auto when = transport.currentFrame + (transport.unitLength - transport.currentFrame % transport.unitLength);
                    scheduleTransition(State::Recording, when);
                }
                break;
            }
            case State::Paused: {
                [[fallthrough]];
            }
            case State::Playback: {
                scheduleTransition(State::Recording, transport.currentFrame + [&] {
                    if (synced) {
                        const auto when = length - phase(transport.currentFrame);
                        return when;
                    } else {
                        return 0;
                    }
                }());
                break;
            }
            default:;
        }
    }

    void LooperProcessor::Track::stopRecording(bool synced) noexcept
    {
        auto& transport = *transportPtr;
        switch (state) {
            case State::Recording: {
                if (isEmpty()) {
                    const auto currentLength = transport.currentFrame - start;
                    if (!transport.isTempoSet()) {
                        if (currentLength > 0) {
                            length = currentLength;
                            state = State::Playback;
                            transport.setTempo(length);
                        }
                    } else {
                        const FrameInt lengthAfterSnap = snapForwardSquareGrid(currentLength, transport.unitLength);
                        scheduleTransition(State::Playback, transport.currentFrame + (lengthAfterSnap - currentLength));
                    }
                } else {
                    scheduleTransition(State::Playback, transport.currentFrame + [&] {
                        if (synced) {
                            const auto framesToLoop = length - phase(transport.currentFrame);
                            return framesToLoop;
                        } else {
                            return 0;
                        }
                    }());
                }
                break;
            }
            default:;
        }
    }

    void LooperProcessor::Track::pause(bool synced) noexcept
    {
        if (state != State::Playback) return;
        const auto currentFrame = transportPtr->currentFrame; 
        scheduleTransition(State::Paused, currentFrame + [&] {
            if (synced) {
                return length - phase(currentFrame);
            } else {
                return 0;
            }
        }());
    }

    void LooperProcessor::Track::resume(bool synced) noexcept
    {
        if (state != State::Paused) return;
        const auto currentFrame = transportPtr->currentFrame; 
        scheduleTransition(State::Playback, currentFrame + [&] {
            if (synced) {
                return length - phase(currentFrame);
            } else {
                return 0;
            }
        }());
    }

    bool LooperProcessor::Track::clear() noexcept
    {
        if (state == State::Cleared) return false;

        const auto toErase = std::min(std::max<FrameInt>(
            transportPtr->currentFrame - start,
            length
        ), static_cast<FrameInt>(buffers[0].size()));

        for (auto& buffer : buffers)
            std::ranges::fill_n(buffer.begin(), toErase, 0.0f);

        hasPendingTransition = false;
        state = State::Cleared;
        start = 0;
        length = 0;

        clearThumbnail();

        return true;
    }

    FrameInt LooperProcessor::Track::getPosition() const noexcept
    {
        return phase(transportPtr->currentFrame);
    }

    FrameInt LooperProcessor::Track::getLength() const noexcept
    {
        return length;
    }

    bool LooperProcessor::Track::isEmpty() const noexcept
    {
        return length == 0;
    }

    void LooperProcessor::Track::scheduleTransition(State next, FrameInt when)
    {
        pendingState = next;
        hasPendingTransition = true;
        whenTransition = when;

        const auto now = transportPtr->currentFrame;
        assert(whenTransition >= now);

        // a hacky way to immediately transition in case no wait time needed
        if (whenTransition == now) {
            handlePendingTransition(whenTransition);
        }
    }

    void LooperProcessor::Track::handlePendingTransition(const FrameInt now) noexcept
    {
        if (!hasPendingTransition) return;
        assert(now <= whenTransition);

        if (now == whenTransition) {
            hasPendingTransition = false;
            switch (pendingState) {
                case State::Recording: {
                    if (isEmpty()) {
                        start = now;
                    }
                    state = State::Recording;
                    break;
                }
                case State::Playback: {
                    if (isEmpty() && state == State::Recording) {
                        length = now - start;
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
    }

    FrameInt LooperProcessor::Track::phase(const FrameInt transportFrame) const noexcept
    {
        if (state == State::Recording && length == 0)
            return transportFrame - start;

        if (length == 0)
            return 0;

        return (transportFrame - start) % length;
    }

    std::pair<float, float> LooperProcessor::Track::getFadeScalars(const FrameInt pos) const noexcept
    {
        float fadeIn = 1.0f;
        float fadeOut = 1.0f;

        if (pos <= fadeLength) {
            fadeIn = static_cast<float>(pos) / static_cast<float>(fadeLength + 1);
        }

        if ((length - pos) <= fadeLength) {
            fadeOut = static_cast<float>(length - pos) / static_cast<float>(fadeLength + 1);
        }

        return std::make_pair(fadeIn, fadeOut);
    }

    void LooperProcessor::Track::updateThumbnail() noexcept
    {
        if (isEmpty()) return;

        const float step = static_cast<float>(length) / ThumbnailSnapshot::kBuckets;
        FrameInt begin = static_cast<FrameInt>(currentBucket * step);
        FrameInt end = static_cast<FrameInt>((currentBucket + 1) * step);
        if (end > length) end = length;
        if (end <= begin) end = begin + 1;

        float minV = 0.0f, maxV = 0.0f;
        for (FrameInt i = begin; i < end; ++i) {
            float l = buffers[0][i];
            float r = buffers[1][i];
            float s = (std::abs(l) > std::abs(r)) ? l : r;
            if (s < minV) minV = s;
            if (s > maxV) maxV = s;
        }

        thumbnail.buckets[currentBucket] = {minV, maxV};

        currentBucket = (currentBucket + 1) % ThumbnailSnapshot::kBuckets;
    }

    void LooperProcessor::Track::clearThumbnail() noexcept
    {
        thumbnail = {};
        currentBucket = 0;
    }
}