#include "midi_message.h"

namespace ml::midi {
    MidiMessage::MidiMessage(const PmMessage msg) : raw_(msg)
    {
        parse();
    }

    PmMessage MidiMessage::raw() const { return raw_; }

    std::uint8_t MidiMessage::status() const { return status_; }
    std::uint8_t MidiMessage::channel() const { return channel_; }
    std::uint8_t MidiMessage::data1() const { return data1_; }
    std::uint8_t MidiMessage::data2() const { return data2_; }

    MidiMessage::Type MidiMessage::type() const { return type_; }

    std::string_view MidiMessage::typeName() const
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

    bool MidiMessage::isNoteOn() const  { return type_ == Type::NoteOn && data2_ > 0; }
    bool MidiMessage::isNoteOff() const { return type_ == Type::NoteOff || (type_ == Type::NoteOn && data2_ == 0); }
    bool MidiMessage::isCC() const { return type_ == Type::ControlChange; }
    bool MidiMessage::isProgram() const { return type_ == Type::ProgramChange; }
    bool MidiMessage::isPitchBend() const { return type_ == Type::PitchBend; }

    std::optional<std::uint8_t> MidiMessage::note() const
    {
        if (type_ == Type::NoteOn || type_ == Type::NoteOff || type_ == Type::PolyAftertouch)
            return data1_;
        return std::nullopt;
    }

    [[nodiscard]] std::optional<std::uint8_t> MidiMessage::velocity() const
    {
        if (type_ == Type::NoteOn || type_ == Type::NoteOff)
            return data2_;
        return std::nullopt;
    }

    [[nodiscard]] std::optional<std::uint8_t> MidiMessage::control() const
    {
        if (type_ == Type::ControlChange)
            return data1_;
        return std::nullopt;
    }

    std::optional<std::uint8_t> MidiMessage::value() const
    {
        if (type_ == Type::ControlChange || type_ == Type::ChannelAftertouch)
            return data2_;
        return std::nullopt;
    }

    std::optional<std::uint8_t> MidiMessage::program() const
    {
        if (type_ == Type::ProgramChange)
            return data1_;
        return std::nullopt;
    }

    std::optional<std::int16_t> MidiMessage::pitchBend() const
    {
        if (type_ == Type::PitchBend) {
            const int value14 = (data2_ << 7) | data1_;
            return static_cast<std::int16_t>(value14 - 8192);
        }
        return std::nullopt;
    }

    void MidiMessage::parse()
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
}