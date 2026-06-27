#pragma once

#include <cstdint>
#include <string_view>
#include <optional>

#include "portmidi_header.h"

#include "threading/spsc_mailbox.h"

namespace ml::midi {
    class MidiMessage
    {
    public:
        enum class Type : std::uint8_t
        {
            NoteOff,
            NoteOn,
            PolyAftertouch,
            ControlChange,
            ProgramChange,
            ChannelAftertouch,
            PitchBend,
            SystemCommon,
            SystemRealtime,
            Unknown
        };

        MidiMessage() = default;

        explicit MidiMessage(const PmMessage msg);

        [[nodiscard]] PmMessage raw() const;

        [[nodiscard]] std::uint8_t status() const;
        [[nodiscard]] std::uint8_t channel() const;
        [[nodiscard]] std::uint8_t data1() const;
        [[nodiscard]] std::uint8_t data2() const;

        [[nodiscard]] Type type() const;
        [[nodiscard]] std::string_view typeName() const;

        [[nodiscard]] bool isNoteOn() const;
        [[nodiscard]] bool isNoteOff() const;
        [[nodiscard]] bool isCC() const;
        [[nodiscard]] bool isProgram() const;
        [[nodiscard]] bool isPitchBend() const;

        [[nodiscard]] std::optional<std::uint8_t> note() const;
        [[nodiscard]] std::optional<std::uint8_t> velocity() const;
        [[nodiscard]] std::optional<std::uint8_t> control() const;
        [[nodiscard]] std::optional<std::uint8_t> value() const;
        [[nodiscard]] std::optional<std::uint8_t> program() const;

        // 14-bit pitch bend centered at 0
        [[nodiscard]] std::optional<std::int16_t> pitchBend() const;

    private:
        void parse();

        PmMessage raw_{0};
        std::uint8_t status_{0};
        std::uint8_t channel_{0};
        std::uint8_t data1_{0};
        std::uint8_t data2_{0};
        Type type_{Type::Unknown};
    };

    using MidiQueue = SpscMailbox<MidiMessage>;
}