#pragma once

#include <functional>

struct Timer
{
    using TimerCallback = std::function<void()>;

    TimerCallback onTimeout{nullptr};
    bool isOneShot{true};
    float timeoutSecs{1.0f};

    Timer() = default;
    Timer(TimerCallback onTimeout, bool isOneshot, float timeoutSecs)
        : onTimeout(std::move(onTimeout)), isOneShot(isOneshot), timeoutSecs(timeoutSecs)
    {}

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
        if (!set_ || !onTimeout) return;

        currentTime_ += dt;
        if (currentTime_ >= timeoutSecs) {
            onTimeout();

            currentTime_ = 0.0f;
            if (isOneShot) set_ = false;
        }
    }

private:
    bool set_{false};
    float currentTime_{0.0f};
};