#pragma once

#include <memory>

#include "looper_processor.h"

namespace looper {
    class LooperCallback;

    class Looper
    {
    public:
        Looper();
        ~Looper() = default;

        Looper(const Looper &) = delete;
        Looper& operator=(const Looper &) = delete;

        Looper(const Looper &&) = delete;
        Looper& operator=(Looper &&) = delete;

        struct LooperState
        {
            unsigned int nFrames;
            unsigned int position;
            LooperProcessor::State state;
        };

        [[nodiscard]] LooperState getLooperState() const noexcept;

        void startRecording();
        void stopRecording();
        void clear();

    private:
        std::shared_ptr<LooperCallback> cb_;
    };
}