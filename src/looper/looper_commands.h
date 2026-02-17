#pragma once

#include <variant>

#include "spsc_mailbox.h"

namespace looper {

class LooperProcessor;

class LooperCommand 
{
public:
    LooperCommand() noexcept : LooperCommand(Dummy{}) {}

    static LooperCommand startRecording() noexcept;
    static LooperCommand stopRecording() noexcept;
    static LooperCommand clear() noexcept;

    void apply(LooperProcessor& looper) const;

private:
    struct Dummy
    {
        void apply(LooperProcessor&) const {}
    };

    struct StartRecording
    {
        void apply(LooperProcessor& looper) const;
    };

    struct StopRecording
    {
        void apply(LooperProcessor& looper) const;
    };

    struct Clear
    {
        void apply(LooperProcessor& looper) const;
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
