#pragma once

#include <functional>

#include "audio/audio_engine.h"
#include "midi_message.h"
#include "timer.h"

namespace midi {
    // Helper to use a midi control message as a footswitch
    class FootSwitch
    {
    public:
        using FootSwitchCallback = std::function<void()>;

        FootSwitch() = default;
        explicit FootSwitch(const uint8_t cc) : cc_(cc) {}

        void setOnSinglePressed(FootSwitchCallback&& callback)
        {
            onSinglePressed_ = std::move(callback);
        }

        void setOnDoublePressed(FootSwitchCallback&& callback)
        {
            onDoublePressed_ = std::move(callback);
        }

        void setOnHold(FootSwitchCallback&& callback)
        {
            holdTimer_.setOnTimeout(std::move(callback));
        }

        void tick() noexcept
        {
            pressTimer_.tick();
            holdTimer_.tick();
        }

        void update(const midi::MidiMessage& msg)
        {
            if (isPressed(msg)) {
                if (onSinglePressed_) onSinglePressed_();

                if (pressTimer_.isRunning()) {
                    if (onDoublePressed_) onDoublePressed_();
                }

                const auto sampleRate = static_cast<float>(audio::AudioEngine::getInstance().getSampleRate());
                pressTimer_.setOneShot(true);
                pressTimer_.setTimeoutSecs(sampleRate, kTimeoutSecs);
                pressTimer_.start();
            }
        }

    private:
        bool isPressed(const midi::MidiMessage& msg)
        {
            if (const auto control = msg.control(); control.has_value()) {
                if (*control != cc_) return false;
                const auto controlValue = *msg.value();
                const bool pressed = (controlValue >= 64);

                if (pressed) {
                    if (!lastPressed_) {
                        const auto sampleRate = static_cast<float>(audio::AudioEngine::getInstance().getSampleRate());
                        holdTimer_.setOneShot(true);
                        holdTimer_.setTimeoutSecs(sampleRate, kHoldTimeSecs);
                        holdTimer_.start();
                    }
                } else {
                    holdTimer_.stop();
                }

                const bool fired = pressed && !lastPressed_;
                lastPressed_ = pressed;
                return fired;
            }
            return false;
        }

        static constexpr float kTimeoutSecs = 0.25f;
        static constexpr float kHoldTimeSecs = 2.0f;

        FootSwitchCallback onSinglePressed_;
        FootSwitchCallback onDoublePressed_;

        timer::AudioRateTimer pressTimer_;
        timer::AudioRateTimer holdTimer_;

        uint8_t cc_{64};
        bool lastPressed_{false};
    };
}