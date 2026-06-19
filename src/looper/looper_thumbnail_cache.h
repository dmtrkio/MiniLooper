#pragma once

#include <array>
#include <chrono>

#include "looper.h"
#include "looper/looper_processor.h"

namespace ml::looper {
    class ThumbnailCache
    {
    public:
        static constexpr int kBuckets = ThumbnailSnapshot::kBuckets;
        static constexpr auto kDefaultInterval = std::chrono::milliseconds(200);

        explicit ThumbnailCache(const Looper& looper, std::chrono::milliseconds interval = kDefaultInterval);

        void update(int trackIndex);

        [[nodiscard]] const ThumbnailSnapshot* get(int trackIndex) const noexcept;

    private:
        const Looper& looper_;
        const std::chrono::milliseconds interval_;

        std::array<ThumbnailSnapshot, kLooperTrackCount> cachedThumbnails_{};
        std::array<std::chrono::steady_clock::time_point, kLooperTrackCount> lastUpdate_{};
    };
}