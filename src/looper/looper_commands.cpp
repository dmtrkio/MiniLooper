#include "looper_commands.h"
#include "looper.h"

namespace looper {

LooperCommand LooperCommand::startRecording() noexcept{ return LooperCommand{ StartRecording{} }; }
LooperCommand LooperCommand::stopRecording() noexcept { return LooperCommand{ StopRecording{} }; }
LooperCommand LooperCommand::clear() noexcept { return LooperCommand{ Clear{} }; }

void LooperCommand::apply(Looper& looper) const
{
    std::visit([&](auto const& c){ c.apply(looper); }, cmd_);
}

void LooperCommand::StartRecording::apply(Looper& looper) const { looper.startRecording(); }
void LooperCommand::StopRecording::apply(Looper& looper) const { looper.stopRecording(); }
void LooperCommand::Clear::apply(Looper& looper) const { looper.clear(); }

} // namespace looper
