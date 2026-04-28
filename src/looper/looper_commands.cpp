#include "looper_commands.h"
#include "looper_processor.h"

namespace looper {
    LooperCommand LooperCommand::startRecording(int trackIndex) noexcept
    {
        return LooperCommand{ StartRecording{ trackIndex } };
    }

    LooperCommand LooperCommand::stopRecording(int trackIndex) noexcept
    {
        return LooperCommand{ StopRecording{ trackIndex } };
    }

    LooperCommand LooperCommand::clear(int trackIndex) noexcept
    {
        return LooperCommand{ Clear{ trackIndex } };
    }

    LooperCommand LooperCommand::pause(int trackIndex) noexcept
    {
        return LooperCommand{ Pause{ trackIndex, false } };
    }

    LooperCommand LooperCommand::pauseAll() noexcept
    {
        return LooperCommand{ Pause{ -1, true } };
    }

    LooperCommand LooperCommand::resume(int trackIndex) noexcept
    {
        return LooperCommand{ Resume{ trackIndex, false } };
    }

    LooperCommand LooperCommand::resumeAll() noexcept
    {
        return LooperCommand{ Resume{ -1, true } };
    }

    LooperCommand LooperCommand::clearAllTracks() noexcept
    {
        return LooperCommand{ ClearAllTracks{} };
    }

    // unlike other commands this one should always be awaited for completion
    LooperCommand LooperCommand::copyLoops(CopyData* copyData, CompletionFlag* completionFlag) noexcept
    {
        assert(copyData && completionFlag);
        auto cmd = LooperCommand{ CopyLoops{ copyData } };
        cmd.addCompletionFlag(completionFlag);
        return cmd;
    }

    void LooperCommand::addCompletionFlag(CompletionFlag* flag) noexcept
    {
        completionFlag_ = flag;
    }

    void LooperCommand::apply(LooperProcessor& looper) const
    {
        std::visit([&](auto const& c){ c.apply(looper); }, cmd_);
        if (completionFlag_) {
            completionFlag_->complete.store(true);
        }
    }

    void LooperCommand::StartRecording::apply(LooperProcessor& looper) const
    {
        looper.startRecording(trackIndex);
    }

    void LooperCommand::StopRecording::apply(LooperProcessor& looper) const
    {
        looper.stopRecording(trackIndex);
    }

    void LooperCommand::Clear::apply(LooperProcessor& looper) const
    {
        looper.clear(trackIndex);
    }

    void LooperCommand::Pause::apply(LooperProcessor& looper) const
    {
        if (all) {
            for (int i = 0; i < looper.getNumLooperTracks(); ++i) {
                looper.pause(i);
            }
        } else {
            looper.pause(trackIndex);
        }
    }

    void LooperCommand::Resume::apply(LooperProcessor& looper) const
    {
        if (all) {
            for (int i = 0; i < looper.getNumLooperTracks(); ++i) {
                looper.resume(i);
            }
        } else {
            looper.resume(trackIndex);
        }
    }

    void LooperCommand::ClearAllTracks::apply(LooperProcessor& looper) const
    {
        looper.clearAll();
    }

    void LooperCommand::CopyLoops::apply(LooperProcessor& looper) const
    {
        if (!copyData) return;
        for (int i = 0; i < static_cast<int>(kLooperTrackCount); ++i) {
            float *data[2] = { copyData->buffersL[i], copyData->buffersR[i] };
            copyData->framesWritten[i] = looper.copyLoop(i, data, copyData->maxFrames);
        }
    }
}