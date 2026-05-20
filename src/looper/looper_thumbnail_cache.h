#pragma once

#include <array>
#include <chrono>
#include <cstring>
#include <atomic>

#include "looper.h"

namespace looper {
    class ThumbnailCache
    {
    public:
        static constexpr int kBuckets = ThumbnailSnapshot::kBuckets;
        static constexpr auto kDefaultInterval = std::chrono::milliseconds(200);

        struct Entry {
            std::pair<float, float> buckets[kBuckets];
            unsigned int length = 0;
            bool valid = false;
        };

        explicit ThumbnailCache(const Looper& looper,
                                std::chrono::milliseconds interval = kDefaultInterval)
            : looper_(looper), interval_(interval) {}

        // Call once per UI frame per track.  Usually does nothing; only blocks
        // when the rate-limit timer has elapsed AND the track is dirty.
        void update(int trackIndex)
        {
            const auto now = std::chrono::steady_clock::now();
            if (now - lastUpdate_[trackIndex] < interval_)
                return;

            const auto& state = looper_.getTrackState(trackIndex);
            auto& entry = entries_[trackIndex];

            bool needs = !entry.valid;
            needs |= (state.nFrames != entry.length);
            needs |= (state.state == State::Recording);
            needs |= dirty_[trackIndex].exchange(false);

            if (!needs)
                return;

            ThumbnailSnapshot snap{};
            looper_.getThumbnail(trackIndex, snap);

            std::memcpy(entry.buckets, snap.buckets, sizeof(entry.buckets));
            entry.length = snap.length;
            entry.valid  = true;
            lastUpdate_[trackIndex] = now;
        }

        // nullptr if no thumbnail has ever been fetched for this track yet.
        const Entry* get(int trackIndex) const noexcept
        {
            return entries_[trackIndex].valid ? &entries_[trackIndex] : nullptr;
        }

        // Force a refresh on the next update() call (safe from any thread).
        void invalidate(int trackIndex) { dirty_[trackIndex].store(true); }
        void invalidateAll() { for (auto& d : dirty_) d.store(true); }

    private:
        const Looper& looper_;
        const std::chrono::milliseconds interval_;

        std::array<Entry, kLooperTrackCount> entries_;
        std::array<std::chrono::steady_clock::time_point, kLooperTrackCount> lastUpdate_{};
        std::array<std::atomic<bool>, kLooperTrackCount> dirty_{};
    };
}