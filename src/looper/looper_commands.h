#pragma once

#include <variant>

#include "spsc_mailbox.h"
#include "atomic_wrapper.h"

namespace looper {

    class LooperProcessor;

    class LooperCommand
    {
    public:
        LooperCommand() noexcept : LooperCommand(Dummy{}) {}

        struct CompletionFlag
        {
            AcquireReleaseAtomic<bool> complete{false};
        };

        static LooperCommand startRecording(int trackIndex) noexcept;
        static LooperCommand stopRecording(int trackIndex) noexcept;
        static LooperCommand clear(int trackIndex) noexcept;
        static LooperCommand pause(int trackIndex) noexcept;
        static LooperCommand pauseAll() noexcept;
        static LooperCommand resume(int trackIndex) noexcept;
        static LooperCommand resumeAll() noexcept;
        static LooperCommand clearAllTracks() noexcept;

        void addCompletionFlag(CompletionFlag* flag) noexcept;
        void apply(LooperProcessor& looper) const;

    private:
        struct Dummy
        {
            void apply(LooperProcessor&) const {}
        };

        struct StartRecording
        {
            int trackIndex;
            void apply(LooperProcessor& looper) const;
        };

        struct StopRecording
        {
            int trackIndex;
            void apply(LooperProcessor& looper) const;
        };

        struct Clear
        {
            int trackIndex;
            void apply(LooperProcessor& looper) const;
        };

        struct Pause
        {
            int trackIndex;
            bool all = false;
            void apply(LooperProcessor& looper) const;
        };

        struct Resume
        {
            int trackIndex;
            bool all = false;
            void apply(LooperProcessor& looper) const;
        };

        struct ClearAllTracks
        {
            void apply(LooperProcessor& looper) const;
        };

        using Variant = std::variant<
            Dummy,
            StartRecording,
            StopRecording,
            Clear,
            Pause,
            Resume,
            ClearAllTracks
        >;

        explicit LooperCommand(Variant cmd) noexcept : cmd_(cmd) {}

        Variant cmd_;
        CompletionFlag* completionFlag_{nullptr};
    };

    using LooperMailbox = SpscMailbox<LooperCommand>;
}