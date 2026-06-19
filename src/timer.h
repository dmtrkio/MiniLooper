#pragma once

#include <functional>
#include <cassert>

namespace ml::timer {
    using TimerCallback = std::function<void()>;

    class Timer
    {
    public:
        Timer() = default;
        Timer(TimerCallback onTimeout, bool isOneshot, float timeoutSecs)
            : onTimeout_(std::move(onTimeout)), isOneShot_(isOneshot), timeoutSecs_(timeoutSecs)
        {
            assert(timeoutSecs_ >= 0.0f);
        }

        void setOnTimeout(TimerCallback onTimeout)
        {
            onTimeout_ = std::move(onTimeout);
        }

        void setOneShot(bool oneShot) noexcept
        {
            isOneShot_ = oneShot;
        }

        void setTimeout(float timeoutSecs) noexcept
        {
            timeoutSecs_ = timeoutSecs;
            assert(timeoutSecs_ >= 0.0f);
        }

        void start() noexcept
        {
            set_ = true;
            currentTime_ = 0.0f;
        }

        void stop() noexcept
        {
            set_ = false;
        }

        [[nodiscard]] bool isRunning() const noexcept
        {
            return set_;
        }

        void tick(float dt)
        {
            if (!set_) return;

            currentTime_ += dt;
            if (currentTime_ >= timeoutSecs_) {
                if (onTimeout_) onTimeout_();

                currentTime_ = 0.0f;
                if (isOneShot_) set_ = false;
            }
        }

    private:
        TimerCallback onTimeout_{nullptr};
        bool isOneShot_{true};
        float timeoutSecs_{1.0f};
        bool set_{false};
        float currentTime_{0.0f};
    };

    class AudioRateTimer
    {
    public:
        AudioRateTimer() = default;
        AudioRateTimer(TimerCallback onTimeout, bool isOneshot, float sampleRate, float timeoutSecs)
            : AudioRateTimer(std::move(onTimeout), isOneshot, calculateFrames(sampleRate, timeoutSecs))
        {}

        AudioRateTimer(TimerCallback onTimeout, bool isOneshot, int frameLimit)
            : onTimeout_(std::move(onTimeout)), isOneShot_(isOneshot), frameLimit_(frameLimit)
        {
            assert(frameLimit_ >= 0);
        }

        void setOnTimeout(TimerCallback onTimeout)
        {
            onTimeout_ = std::move(onTimeout);
        }

        void setOneShot(bool oneShot) noexcept
        {
            isOneShot_ = oneShot;
        }

        void setTimeoutFrames(int nFrames) noexcept
        {
            frameLimit_ = nFrames;
            assert(frameLimit_ >= 0);
        }

        void setTimeoutSecs(float sampleRate, float timeoutSecs) noexcept
        {
            setTimeoutFrames(calculateFrames(sampleRate, timeoutSecs));
        }

        void start() noexcept
        {
            set_ = true;
            framesLeft_ = frameLimit_;
        }

        void stop() noexcept
        {
            set_ = false;
        }

        [[nodiscard]] bool isRunning() const noexcept
        {
            return set_;
        }

        // call on each audio frame
        void tick() noexcept
        {
            if (!set_) return;

            framesLeft_--;
            if (framesLeft_ <= 0) {
                if (onTimeout_) onTimeout_();

                if (isOneShot_) {
                    set_ = false;
                } else {
                    framesLeft_ = frameLimit_;
                }
            }
        }

    private:
        static int calculateFrames(float sampleRate, float timeSecs) noexcept
        {
            return static_cast<int>(timeSecs * sampleRate);
        }

        TimerCallback onTimeout_{nullptr};
        bool isOneShot_{true};
        int frameLimit_{0};
        bool set_{false};
        int framesLeft_{0};
    };
}