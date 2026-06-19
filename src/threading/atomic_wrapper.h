#pragma once

#include <atomic>
#include <type_traits>

namespace ml {
    template<typename T, std::memory_order kReadMemoryOrder, std::memory_order kWriteMemoryOrder>
    class AtomicWrapper
    {
        static_assert(std::is_trivially_copyable_v<T>,
                    "T is required to be trivially copyable");

        static_assert(std::atomic<T>::is_always_lock_free,
                    "atomic<T> is required to be lock-free");

    public:
        AtomicWrapper() noexcept = default;

        explicit AtomicWrapper(T initial) noexcept
        {
            value.store(initial, kWriteMemoryOrder);
        }

        AtomicWrapper(const AtomicWrapper& other) noexcept
        {
            value.store(other.value.load(kReadMemoryOrder), kWriteMemoryOrder);
        }

        AtomicWrapper& operator=(const AtomicWrapper& other) noexcept
        {
            value.store(other.value.load(kReadMemoryOrder), kWriteMemoryOrder);
            return *this;
        }

        AtomicWrapper& operator=(T v) noexcept
        {
            value.store(v, kWriteMemoryOrder);
            return *this;
        }

        operator T() const noexcept
        {
            return load();
        }

        T load() const noexcept
        {
            return value.load(kReadMemoryOrder);
        }

        void store(T v) noexcept
        {
            value.store(v, kWriteMemoryOrder);
        }

        bool operator==(const AtomicWrapper& other) const noexcept
        {
            return load() == other.load();
        }

        bool operator!=(const AtomicWrapper& other) const noexcept
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

    template <typename T>
    using RelaxedAtomic = AtomicWrapper<T, std::memory_order_relaxed, std::memory_order_relaxed>;

    template <typename T>
    using AcquireReleaseAtomic = AtomicWrapper<T, std::memory_order_acquire, std::memory_order_release>;
}