#pragma once

#include <array>
#include <bitset>
#include <chrono>

#include "looper.h"
#include "looper/looper_processor.h"

namespace looper {
    class ThumbnailCache
    {
    public:
        static constexpr int kBuckets = ThumbnailSnapshot::kBuckets;
        static constexpr auto kDefaultInterval = std::chrono::milliseconds(200);

        explicit ThumbnailCache(const Looper& looper, std::chrono::milliseconds interval = kDefaultInterval)
            : looper_(looper)
            , interval_(interval)
        {}

        void update(int trackIndex)
        {
            const auto now = std::chrono::steady_clock::now();
            if (now - lastUpdate_[trackIndex] < interval_)
                return;

            const auto& state = looper_.getTrackState(trackIndex);
            const auto lastState = lastKnownStates_[trackIndex];
            lastKnownStates_[trackIndex] = state.state;
            auto& entry = cachedThumbnails_[trackIndex];
            auto fetchedBefore = fetched_[trackIndex];

            bool needs = !fetchedBefore;
            needs |= (state.state == State::Recording);
            needs |= (state.state != lastState);

            if (!needs)
                return;

            ThumbnailSnapshot snap{};
            looper_.getThumbnail(trackIndex, snap);

            entry = snap;
            fetched_[trackIndex] = true;
            lastUpdate_[trackIndex] = now;
        }

        [[nodiscard]] const ThumbnailSnapshot* get(int trackIndex) const noexcept
        {
            if (trackIndex < 0 || trackIndex >= kLooperTrackCount) {
                assert(false && "Invalid track index");
                return nullptr;
            }
            return fetched_[trackIndex] ? &cachedThumbnails_[trackIndex] : nullptr;
        }

    private:
        const Looper& looper_;
        const std::chrono::milliseconds interval_;

        std::bitset<kLooperTrackCount> fetched_{};
        std::array<ThumbnailSnapshot, kLooperTrackCount> cachedThumbnails_{};
        std::array<State, kLooperTrackCount> lastKnownStates_{};
        std::array<std::chrono::steady_clock::time_point, kLooperTrackCount> lastUpdate_{};
    };
}