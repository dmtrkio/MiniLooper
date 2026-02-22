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

        struct LooperState
        {
            unsigned int nFrames;
            unsigned int position;
            LooperProcessor::State state;

            void printState() const noexcept
            {
                std::cout << "nFrames: " << nFrames << std::endl;
                std::cout << "position: " << position << std::endl;
                std::cout << "state: " << LooperProcessor::stateToStr(state) << std::endl;
            }
        };

        [[nodiscard]] LooperState getLooperState(int trackIndex) const noexcept;

        void startRecording(int trackIndex);
        void stopRecording(int trackIndex);
        void clear(int trackIndex);
        void pause(int trackIndex);
        void resume(int trackIndex);
        void clearAll();

    private:
        class LooperCallback;
        std::shared_ptr<LooperCallback> cb_;
    };

}