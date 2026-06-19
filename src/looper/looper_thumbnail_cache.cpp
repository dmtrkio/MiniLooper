#include "looper_thumbnail_cache.h"

#include <cassert>

namespace looper {
    ThumbnailCache::ThumbnailCache(const Looper& looper, std::chrono::milliseconds interval)
        : looper_(looper)
        , interval_(interval)
    {}

    void ThumbnailCache::update(int trackIndex)
    {
        const auto now = std::chrono::steady_clock::now();
        if (now - lastUpdate_[trackIndex] < interval_) return;

        auto& entry = cachedThumbnails_[trackIndex];
        looper_.getThumbnail(trackIndex, entry);

        lastUpdate_[trackIndex] = now;
    }

    const ThumbnailSnapshot* ThumbnailCache::get(int trackIndex) const noexcept
    {
        if (trackIndex < 0 || trackIndex >= kLooperTrackCount) {
            assert(false && "Invalid track index");
            return nullptr;
        }
        return &cachedThumbnails_[trackIndex];
    }
}