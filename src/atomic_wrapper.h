#pragma once

#include <atomic>
#include <type_traits>

template<typename T>
class RelaxedAtomic
{
    static_assert(std::is_trivially_copyable_v<T>,
                  "T is required to be trivially copyable");

    static_assert(std::atomic<T>::is_always_lock_free,
                  "atomic<T> is required to be lock-free");

public:
    RelaxedAtomic() noexcept = default;

    explicit RelaxedAtomic(T initial) noexcept
    {
        value.store(initial, std::memory_order_relaxed);
    }

    RelaxedAtomic(const RelaxedAtomic& other) noexcept
    {
        value.store(other.value.load(std::memory_order_relaxed),
                    std::memory_order_relaxed);
    }

    RelaxedAtomic& operator=(const RelaxedAtomic& other) noexcept
    {
        value.store(other.value.load(std::memory_order_relaxed),
                    std::memory_order_relaxed);
        return *this;
    }

    RelaxedAtomic& operator=(T v) noexcept
    {
        value.store(v, std::memory_order_relaxed);
        return *this;
    }

    operator T() const noexcept
    {
        return value.load(std::memory_order_relaxed);
    }

    T load() const noexcept
    {
        return value.load(std::memory_order_relaxed);
    }

    void store(T v) noexcept
    {
        value.store(v, std::memory_order_relaxed);
    }

    T exchange(T v) noexcept
    {
        return value.exchange(v, std::memory_order_relaxed);
    }

    bool operator==(const RelaxedAtomic& other) const noexcept
    {
        return load() == other.load();
    }

    bool operator!=(const RelaxedAtomic& other) const noexcept
    {
        return load() != other.load();
    }

    bool operator==(T other) const noexcept
    {
        return load() == other;
    }

    bool operator!=(T other) const noexcept
    {
        return load() != other;
    }

private:
    std::atomic<T> value{T{}};
};