#include "looper_commands.h"
#include "looper_processor.h"

namespace looper {

LooperCommand LooperCommand::startRecording() noexcept{ return LooperCommand{ StartRecording{} }; }
LooperCommand LooperCommand::stopRecording() noexcept { return LooperCommand{ StopRecording{} }; }
LooperCommand LooperCommand::clear() noexcept { return LooperCommand{ Clear{} }; }

void LooperCommand::apply(LooperProcessor& looper) const
{
    std::visit([&](auto const& c){ c.apply(looper); }, cmd_);
}

void LooperCommand::StartRecording::apply(LooperProcessor& looper) const { looper.startRecording(); }
void LooperCommand::StopRecording::apply(LooperProcessor& looper) const { looper.stopRecording(); }
void LooperCommand::Clear::apply(LooperProcessor& looper) const { looper.clear(); }

} // namespace looper
