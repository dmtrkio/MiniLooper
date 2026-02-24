#pragma once

#include <memory>

#include "looper_processor.h"

namespace looper {

    class Looper
    {
    public:
        Looper();
        ~Looper() = default;

        Looper(const Looper &) = delete;
        Looper& operator=(const Looper &) = delete;

        Looper(const Looper &&) = delete;
        Looper& operator=(Looper &&) = delete;

        [[nodiscard]] int getNumLooperTracks() const noexcept;

        void updateSnapshot() noexcept;
        [[nodiscard]] TrackStateSnapshot getTrackState(int trackIndex) const noexcept;

        void startRecording(int trackIndex);
        void stopRecording(int trackIndex);
        void clear(int trackIndex);
        void pause(int trackIndex);
        void resume(int trackIndex);
        void clearAll();

    private:
        class LooperCallback;
        std::shared_ptr<LooperCallback> cb_;
        LooperStateSnapshot snapshot_;
    };

}