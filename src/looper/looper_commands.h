#pragma once

#include <variant>

#include "spsc_mailbox.h"

namespace looper {

class Looper;

class LooperCommand 
{
public:
    LooperCommand() noexcept : LooperCommand(Dummy{}) {}

    static LooperCommand startRecording() noexcept;
    static LooperCommand stopRecording() noexcept;
    static LooperCommand clear() noexcept;

    void apply(Looper& looper) const;

private:
    struct Dummy
    {
        void apply(Looper&) const {}
    };

    struct StartRecording
    {
        void apply(Looper& looper) const;
    };

    struct StopRecording
    {
        void apply(Looper& looper) const;
    };

    struct Clear
    {
        void apply(Looper& looper) const;
    };

    using Variant = std::variant<
        Dummy,
        StartRecording,
        StopRecording,
        Clear
    >;

    explicit LooperCommand(Variant cmd) noexcept : cmd_(cmd) {}

    Variant cmd_;
};

using LooperMailbox = SpscMailbox<LooperCommand>;

}
