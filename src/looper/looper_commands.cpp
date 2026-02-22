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
        return LooperCommand{ Pause{ trackIndex } };
    }

    LooperCommand LooperCommand::resume(int trackIndex) noexcept
    {
        return LooperCommand{ Resume{ trackIndex } };
    }

    LooperCommand LooperCommand::clearAllTracks() noexcept
    {
        return LooperCommand{ ClearAllTracks{} };
    }

    void LooperCommand::apply(LooperProcessor& looper) const
    {
        std::visit([&](auto const& c){ c.apply(looper); }, cmd_);
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
        looper.pause(trackIndex);
    }

    void LooperCommand::Resume::apply(LooperProcessor& looper) const
    {
        looper.resume(trackIndex);
    }

    void LooperCommand::ClearAllTracks::apply(LooperProcessor& looper) const
    {
        looper.clearAll();
    }

} // namespace looper
