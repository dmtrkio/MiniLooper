#pragma once

#include <array>
#include <climits>
#include <vector>
#include <utility>
#include <cstdint>
#include <concepts>
#include <optional>

#include "looper_mixer.h"
#include "dsp/dsp.h"
#include "dsp/click.h"
#include "rt_sanitizer.h"

namespace ml::looper {
    using FrameInt = std::int32_t;

    inline constexpr int kLooperTrackCount = 4;
    inline constexpr float kMaxLoopSecs = 40.0f;

    struct ThumbnailSnapshot
    {
        ThumbnailSnapshot() = default;
        ThumbnailSnapshot(const ThumbnailSnapshot&) = default;
        ThumbnailSnapshot& operator=(const ThumbnailSnapshot&) = default;
        ThumbnailSnapshot(ThumbnailSnapshot&&) = default;
        ThumbnailSnapshot& operator=(ThumbnailSnapshot&&) = default;

        static constexpr int kBuckets = 200;
        dsp::MinMax buckets[kBuckets];
    };

    enum class State
    {
        Cleared,
        Recording,
        Playback,
        Paused,
    };

    const char* stateToStr(State state);

    class LooperProcessor
    {
    public:
        LooperProcessor();

        // lifetime callbacks
        void prepare(float sampleRate);
        void process(float *const *data, FrameInt nFrames) noexcept RT_SAN;

        [[nodiscard]] constexpr int getNumLooperTracks() const noexcept
        {
            return static_cast<int>(tracks_.size());
        }

        // All the methods below are not thread-safe, they are meant to be used in the same thread where process() is called

        [[nodiscard]] Mixer& getMixer() noexcept;
        [[nodiscard]] State getState(int trackIndex) const noexcept;
        [[nodiscard]] FrameInt getMaxFramesInLoop() const noexcept;
        [[nodiscard]] FrameInt getCurrentPosition(int trackIndex) const noexcept;
        [[nodiscard]] FrameInt getCurrentNumFrames(int trackIndex) const noexcept;
        [[nodiscard]] bool isEmpty(int trackIndex) const noexcept;
        [[nodiscard]] std::optional<float> getApproxBpm() const noexcept;

        void setClickGain(float gain) noexcept;
        void setClickEnabled(bool enabled) noexcept;

        void startRecording(int trackIndex, bool synced = true) noexcept;
        void stopRecording(int trackIndex, bool synced = true) noexcept;
        void clear(int trackIndex) noexcept;
        void pause(int trackIndex, bool synced = true) noexcept;
        void resume(int trackIndex, bool synced = true) noexcept;
        void clearAll() noexcept;
        [[nodiscard]] FrameInt copyLoop(int trackIndex, float *const *data, FrameInt capacity) const noexcept;
        void extractThumbnail(int trackIndex, ThumbnailSnapshot& out) const noexcept;

    private:
        static constexpr float kFadeLengthMs = 5.0f;

        [[nodiscard]] constexpr bool isTrackIndexValid(int trackIndex) const noexcept
        {
            return trackIndex >= 0 && trackIndex < getNumLooperTracks();
        }

        [[nodiscard]] bool isAnyTrackCurrentlyRecording() const noexcept;

        struct Transport
        {
            [[nodiscard]] bool isTempoSet() const noexcept;
            [[nodiscard]] std::optional<float> getApproxBPM() const noexcept;
            bool tick() noexcept;
            void setTempo(FrameInt firstLoopLength) noexcept;
            void reset(FrameInt maxFrameCount, float sr) noexcept;

            float sampleRate;
            FrameInt maxFrames{};
            FrameInt currentFrame{};
            FrameInt unitLength{};
            FrameInt largestPossibleLoopLength{};
        };

        Transport transport_;

        float sampleRate_{44100.0f};
        FrameInt maxFrames_{0};

        struct Track
        {
            void init(Transport* transport, FrameInt maxFrames, float sampleRate) noexcept;
            void process(const float *const *in, float *const *out, const FrameInt nFrames) noexcept;

            void startRecording(bool synced) noexcept;
            void stopRecording(bool synced) noexcept;
            void pause(bool synced) noexcept;
            void resume(bool synced) noexcept;
            bool clear() noexcept;

            [[nodiscard]] FrameInt getPosition() const noexcept;
            [[nodiscard]] FrameInt getLength() const noexcept;

            [[nodiscard]] bool isEmpty() const noexcept;
            void scheduleTransition(State next, FrameInt when);
            void handlePendingTransition(FrameInt now) noexcept;
            [[nodiscard]] FrameInt phase(FrameInt transportFrame) const noexcept;
            [[nodiscard]] std::pair<float, float> getFadeScalars(FrameInt pos) const noexcept;
            void updateThumbnail() noexcept;
            void clearThumbnail() noexcept;

            Transport* transportPtr;

            State state{State::Cleared};
            FrameInt start{0};
            FrameInt length{0};

            State pendingState{State::Cleared};
            bool hasPendingTransition{false};
            FrameInt whenTransition{0};

            FrameInt fadeLength{64};
            std::array<std::vector<float>, 2> buffers;

            ThumbnailSnapshot thumbnail;
            FrameInt currentBucket{0};
        };

        std::array<Track, kLooperTrackCount> tracks_{};

        Mixer mixer_;

        dsp::ClickGenerator click_;
        bool clickEnabled_{false};
        dsp::FloatSmoother clickGain_;
    };

    template <typename T>
    requires std::signed_integral<T> && ((sizeof(T) * CHAR_BIT) >= 32)
    [[nodiscard]] constexpr T snapForwardSquareGrid(const T value, const T grid) noexcept
    {
        if (value <= 0 || grid <= 0) return grid;
        if (value <= grid) return grid;
        
        T ratio = (value + grid - 1) / grid;
        
        ratio--;
        ratio |= ratio >> 1;
        ratio |= ratio >> 2;
        ratio |= ratio >> 4;
        ratio |= ratio >> 8;
        ratio |= ratio >> 16;
        ratio++;
        
        return ratio * grid;
    }

    [[nodiscard]] inline FrameInt estimateQuarterNoteUnit(FrameInt firstLoopLength, float sampleRate) noexcept
    {
        if (firstLoopLength <= 0) return 0;

        const float loopSeconds = static_cast<float>(firstLoopLength) / sampleRate;

        constexpr float kMinBPM = 50.0f;
        constexpr float kMaxBPM = 240.0f;
        constexpr float kTargetBPM = 140.0f;

        int bestK = 0;
        float bestScore = std::numeric_limits<float>::max();
        bool found = false;

        for (int k = -2; k <= 6; ++k) {
            const float n = std::exp2f(static_cast<float>(k));
            const float bpm = 240.0f * n / loopSeconds;

            if (bpm >= kMinBPM && bpm <= kMaxBPM) {
                const float score = std::abs(bpm - kTargetBPM);
                if (score < bestScore) {
                    bestScore = score;
                    bestK = k;
                    found = true;
                }
            }
        }

        if (found) {
            const float n = std::exp2f(static_cast<float>(bestK));
            const FrameInt unit = static_cast<FrameInt>(std::round(
                static_cast<float>(firstLoopLength) / (4.0f * n)
            ));
            return (unit < 1) ? 1 : unit;
        }

        return firstLoopLength;
    }
}