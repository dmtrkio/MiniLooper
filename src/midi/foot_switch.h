#pragma once

#include "midi_message.h"

namespace midi {

    // Helper to use a midi control message as a footswitch
    class FootSwitch
    {
    public:
        FootSwitch() = default;
        explicit FootSwitch(const uint8_t cc) : cc_(cc) {}

        bool update(const midi::MidiMessage& msg)
        {
            if (const auto control = msg.controller(); control.has_value()) {
                if (*control != cc_) return false;
                const auto controlValue = *msg.value();
                const bool pressed = (controlValue >= 64);
                const bool fired = pressed && !lastPressed_;
                lastPressed_ = pressed;
                return fired;
            }

            return false;
        }

    private:
        uint8_t cc_{64};
        bool lastPressed_{false};
    };

}