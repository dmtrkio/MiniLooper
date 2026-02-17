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

    void LooperCommand::apply(LooperProcessor& looper) const
    {
        std::visit([&](auto const& c){ c.apply(looper); }, cmd_);
    }

    void LooperCommand::StartRecording::apply(LooperProcessor& looper) const { looper.startRecording(); }
    void LooperCommand::StopRecording::apply(LooperProcessor& looper) const { looper.stopRecording(); }
    void LooperCommand::Clear::apply(LooperProcessor& looper) const { looper.clear(); }

} // namespace looper
