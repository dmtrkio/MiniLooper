#pragma once

#include <array>
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
            if (now - lastUpdate_[trackIndex] < interval_) return;

            auto& entry = cachedThumbnails_[trackIndex];
            looper_.getThumbnail(trackIndex, entry);

            lastUpdate_[trackIndex] = now;
        }

        [[nodiscard]] const ThumbnailSnapshot* get(int trackIndex) const noexcept
        {
            if (trackIndex < 0 || trackIndex >= kLooperTrackCount) {
                assert(false && "Invalid track index");
                return nullptr;
            }
            return &cachedThumbnails_[trackIndex];
        }

    private:
        const Looper& looper_;
        const std::chrono::milliseconds interval_;

        std::array<ThumbnailSnapshot, kLooperTrackCount> cachedThumbnails_{};
        std::array<std::chrono::steady_clock::time_point, kLooperTrackCount> lastUpdate_{};
    };
}