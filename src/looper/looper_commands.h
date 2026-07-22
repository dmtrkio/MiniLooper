#pragma once

#include <variant>

#include "threading/spsc_mailbox.h"
#include "threading/atomic_wrapper.h"
#include "looper_processor.h"

namespace ml::looper {
    class LooperProcessor;

    class LooperCommand
    {
    public:
        LooperCommand() noexcept : LooperCommand(Dummy{}) {}

        struct CompletionFlag
        {
            AcquireReleaseAtomic<bool> complete{false};
        };

        struct CopyData
        {
            // capacity of pre-allocated buffers
            FrameInt maxFrames;
            // pre-allocated buffers, buffer count should be same as kLooperTrackCount
            float **buffersL;
            float **buffersR;
            // return value
            FrameInt *framesWritten;
        };

        [[nodiscard]] static LooperCommand pauseTransport() noexcept;
        [[nodiscard]] static LooperCommand playTransport() noexcept;
        [[nodiscard]] static LooperCommand stopTransport() noexcept;
        [[nodiscard]] static LooperCommand startRecording(int trackIndex, bool synced = true) noexcept;
        [[nodiscard]] static LooperCommand stopRecording(int trackIndex, bool synced = true) noexcept;
        [[nodiscard]] static LooperCommand clear(int trackIndex) noexcept;
        [[nodiscard]] static LooperCommand pause(int trackIndex, bool synced = true) noexcept;
        [[nodiscard]] static LooperCommand pauseAll() noexcept;
        [[nodiscard]] static LooperCommand resume(int trackIndex, bool synced = true) noexcept;
        [[nodiscard]] static LooperCommand resumeAll() noexcept;
        [[nodiscard]] static LooperCommand clearAllTracks() noexcept;
        // unlike other commands those should always be awaited for completion
        [[nodiscard]] static LooperCommand copyLoops(CopyData& copyData, CompletionFlag& completionFlag) noexcept;
        [[nodiscard]] static LooperCommand getThumbnail(int trackIndex, ThumbnailSnapshot& out, CompletionFlag& completionFlag) noexcept;

        void addCompletionFlag(CompletionFlag* flag) noexcept;
        void apply(LooperProcessor& looper) const;

    private:
        struct Dummy
        {
            void apply(LooperProcessor&) const {}
        };

        struct PauseTransport
        {
            void apply(LooperProcessor& looper) const;
        };

        struct PlayTransport
        {
            void apply(LooperProcessor& looper) const;
        };

        struct StopTransport
        {
            void apply(LooperProcessor& looper) const;
        };

        struct StartRecording
        {
            int trackIndex;
            bool synced = true;
            void apply(LooperProcessor& looper) const;
        };

        struct StopRecording
        {
            int trackIndex;
            bool synced = true;
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
            bool synced = true; 
            void apply(LooperProcessor& looper) const;
        };

        struct Resume
        {
            int trackIndex;
            bool all = false;
            bool synced = true;
            void apply(LooperProcessor& looper) const;
        };

        struct ClearAllTracks
        {
            void apply(LooperProcessor& looper) const;
        };

        struct CopyLoops
        {
            CopyData* copyData;
            void apply(LooperProcessor& looper) const;
        };

        struct GetThumbnail
        {
            int trackIndex;
            ThumbnailSnapshot* out;
            void apply(LooperProcessor& looper) const;
        };

        using Variant = std::variant<
            Dummy,
            PauseTransport,
            PlayTransport,
            StopTransport,
            StartRecording,
            StopRecording,
            Clear,
            Pause,
            Resume,
            ClearAllTracks,
            CopyLoops,
            GetThumbnail
        >;

        explicit LooperCommand(Variant cmd) noexcept : cmd_(cmd) {}

        Variant cmd_;
        CompletionFlag* completionFlag_{nullptr};
    };

    using LooperMailbox = SpscMailbox<LooperCommand>;
}