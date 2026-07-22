#include "looper_commands.h"
#include "looper_processor.h"

namespace ml::looper {
    LooperCommand LooperCommand::pauseTransport() noexcept
    {
        return LooperCommand{ PauseTransport{} };
    }

    LooperCommand LooperCommand::playTransport() noexcept
    {
        return LooperCommand{ PlayTransport{} };
    }

    LooperCommand LooperCommand::stopTransport() noexcept
    {
        return LooperCommand{ StopTransport{} };
    }

    LooperCommand LooperCommand::startRecording(int trackIndex, bool synced) noexcept
    {
        return LooperCommand{ StartRecording{ trackIndex, synced } };
    }

    LooperCommand LooperCommand::stopRecording(int trackIndex, bool synced) noexcept
    {
        return LooperCommand{ StopRecording{ trackIndex, synced } };
    }

    LooperCommand LooperCommand::clear(int trackIndex) noexcept
    {
        return LooperCommand{ Clear{ trackIndex } };
    }

    LooperCommand LooperCommand::pause(int trackIndex, bool synced) noexcept
    {
        return LooperCommand{ Pause{ trackIndex, false, synced } };
    }

    LooperCommand LooperCommand::pauseAll() noexcept
    {
        return LooperCommand{ Pause{ -1, true, true } };
    }

    LooperCommand LooperCommand::resume(int trackIndex, bool synced) noexcept
    {
        return LooperCommand{ Resume{ trackIndex, false, synced } };
    }

    LooperCommand LooperCommand::resumeAll() noexcept
    {
        return LooperCommand{ Resume{ -1, true, true } };
    }

    LooperCommand LooperCommand::clearAllTracks() noexcept
    {
        return LooperCommand{ ClearAllTracks{} };
    }

    LooperCommand LooperCommand::copyLoops(CopyData& copyData, CompletionFlag& completionFlag) noexcept
    {
        auto cmd = LooperCommand{ CopyLoops{ &copyData } };
        cmd.addCompletionFlag(&completionFlag);
        return cmd;
    }

    LooperCommand LooperCommand::getThumbnail(int trackIndex, ThumbnailSnapshot& out, CompletionFlag& completionFlag) noexcept
    {
        auto cmd = LooperCommand{ GetThumbnail{ trackIndex, &out } };
        cmd.addCompletionFlag(&completionFlag);
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

    void LooperCommand::PauseTransport::apply(LooperProcessor& looper) const
    {
        looper.pauseTransport();
    }

    void LooperCommand::PlayTransport::apply(LooperProcessor& looper) const
    {
        looper.playTransport();
    }

    void LooperCommand::StopTransport::apply(LooperProcessor& looper) const
    {
        looper.stopTransport();
    }

    void LooperCommand::StartRecording::apply(LooperProcessor& looper) const
    {
        looper.startRecording(trackIndex, synced);
    }

    void LooperCommand::StopRecording::apply(LooperProcessor& looper) const
    {
        looper.stopRecording(trackIndex, synced);
    }

    void LooperCommand::Clear::apply(LooperProcessor& looper) const
    {
        looper.clear(trackIndex);
    }

    void LooperCommand::Pause::apply(LooperProcessor& looper) const
    {
        if (all) {
            for (int i = 0; i < looper.getNumLooperTracks(); ++i) {
                looper.pause(i, synced);
            }
        } else {
            looper.pause(trackIndex, synced);
        }
    }

    void LooperCommand::Resume::apply(LooperProcessor& looper) const
    {
        if (all) {
            for (int i = 0; i < looper.getNumLooperTracks(); ++i) {
                looper.resume(i, synced);
            }
        } else {
            looper.resume(trackIndex, synced);
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

    void LooperCommand::GetThumbnail::apply(LooperProcessor& looper) const
    {
        if (!out) return;
        looper.extractThumbnail(trackIndex, *out);
    }
}