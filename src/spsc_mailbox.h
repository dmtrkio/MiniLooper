#pragma once

#include <readerwritercircularbuffer.h>

template <typename T>
class SpscMailbox
{
public:
    explicit SpscMailbox(std::size_t capacity)
        : queue_(capacity)
    {}

    SpscMailbox(SpscMailbox const&) = delete;
    SpscMailbox& operator=(SpscMailbox const&) = delete;

    // Producer
    bool tryPush(const T& value) noexcept
    {
        return queue_.try_enqueue(value);
    }

    bool tryPush(T&& value) noexcept
    {
        return queue_.try_enqueue(std::move(value));
    }

    // Consumer side (wait-free)
    template <typename Fn>
    void consumeAll(Fn&& fn) noexcept
    {
        T value;
        while (queue_.try_dequeue(value)) {
            fn(value);
        }
    }

    bool tryPop(T& out) noexcept
    {
        return queue_.try_dequeue(out);
    }

    std::size_t approxSize() const noexcept
    {
        return queue_.size_approx();
    }

    std::size_t capacity() const noexcept
    {
        return queue_.max_capacity();
    }

private:
    moodycamel::BlockingReaderWriterCircularBuffer<T> queue_;
};
