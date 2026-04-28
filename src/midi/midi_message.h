#pragma once

#include <cstdint>
#include <string>
#include <optional>

#include "portmidi.h"

#include "threading/spsc_mailbox.h"

namespace midi {
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

        explicit MidiMessage(const PmMessage msg) : raw_(msg)
        {
            parse();
        }

        [[nodiscard]] PmMessage raw() const { return raw_; }

        [[nodiscard]] std::uint8_t status() const { return status_; }
        [[nodiscard]] std::uint8_t channel() const { return channel_; }
        [[nodiscard]] std::uint8_t data1() const { return data1_; }
        [[nodiscard]] std::uint8_t data2() const { return data2_; }

        [[nodiscard]] Type type() const { return type_; }

        [[nodiscard]] bool isNoteOn() const  { return type_ == Type::NoteOn && data2_ > 0; }
        [[nodiscard]] bool isNoteOff() const { return type_ == Type::NoteOff || (type_ == Type::NoteOn && data2_ == 0); }
        [[nodiscard]] bool isCC() const { return type_ == Type::ControlChange; }
        [[nodiscard]] bool isProgram() const { return type_ == Type::ProgramChange; }
        [[nodiscard]] bool isPitchBend() const { return type_ == Type::PitchBend; }

        [[nodiscard]] std::optional<std::uint8_t> note() const
        {
            if (type_ == Type::NoteOn || type_ == Type::NoteOff || type_ == Type::PolyAftertouch)
                return data1_;
            return std::nullopt;
        }

        [[nodiscard]] std::optional<std::uint8_t> velocity() const
        {
            if (type_ == Type::NoteOn || type_ == Type::NoteOff)
                return data2_;
            return std::nullopt;
        }

        [[nodiscard]] std::optional<std::uint8_t> control() const
        {
            if (type_ == Type::ControlChange)
                return data1_;
            return std::nullopt;
        }

        [[nodiscard]] std::optional<std::uint8_t> value() const
        {
            if (type_ == Type::ControlChange || type_ == Type::ChannelAftertouch)
                return data2_;
            return std::nullopt;
        }

        [[nodiscard]] std::optional<std::uint8_t> program() const
        {
            if (type_ == Type::ProgramChange)
                return data1_;
            return std::nullopt;
        }

        // 14-bit pitch bend centered at 0
        [[nodiscard]] std::optional<std::int16_t> pitchBend() const
        {
            if (type_ == Type::PitchBend) {
                const int value14 = (data2_ << 7) | data1_;
                return static_cast<std::int16_t>(value14 - 8192);
            }
            return std::nullopt;
        }

        [[nodiscard]] std::string typeName() const
        {
            switch (type_) {
                case Type::NoteOff: return "NoteOff";
                case Type::NoteOn: return "NoteOn";
                case Type::PolyAftertouch: return "PolyAftertouch";
                case Type::ControlChange: return "ControlChange";
                case Type::ProgramChange: return "ProgramChange";
                case Type::ChannelAftertouch: return "ChannelAftertouch";
                case Type::PitchBend: return "PitchBend";
                case Type::SystemCommon: return "SystemCommon";
                case Type::SystemRealtime: return "SystemRealtime";
                default: return "Unknown";
            }
        }

    private:
        void parse()
        {
            status_ = raw_ & 0xFF;
            data1_  = (raw_ >> 8) & 0xFF;
            data2_  = (raw_ >> 16) & 0xFF;

            if ((status_ & 0xF0) != 0xF0) {
                channel_ = status_ & 0x0F;
                const std::uint8_t code = status_ & 0xF0;

                switch (code) {
                    case 0x80: type_ = Type::NoteOff; break;
                    case 0x90: type_ = Type::NoteOn; break;
                    case 0xA0: type_ = Type::PolyAftertouch; break;
                    case 0xB0: type_ = Type::ControlChange; break;
                    case 0xC0: type_ = Type::ProgramChange; break;
                    case 0xD0: type_ = Type::ChannelAftertouch; break;
                    case 0xE0: type_ = Type::PitchBend; break;
                    default:   type_ = Type::Unknown; break;
                }
            } else {
                channel_ = 0;
                if (status_ >= 0xF8)
                    type_ = Type::SystemRealtime;
                else
                    type_ = Type::SystemCommon;
            }
        }

        PmMessage raw_{0};
        std::uint8_t status_{0};
        std::uint8_t channel_{0};
        std::uint8_t data1_{0};
        std::uint8_t data2_{0};
        Type type_{Type::Unknown};
    };

    using MidiQueue = SpscMailbox<MidiMessage>;
}