#pragma once

/* Single producer, single consumer triple buffer based on https://github.com/brilliantsugar/trio */

#include <atomic>

template <typename T>
class TripleBuffer
{
public:
    using DataType = T;

    TripleBuffer() = default;

    explicit TripleBuffer(DataType&& initial_front_buf)
        : buffers_{{}, {}, {std::forward<DataType>(initial_front_buf)}}
    {}

    TripleBuffer(DataType&& initial_back_buf,
         DataType&& initial_middle_buf,
         DataType&& initial_front_buf)
        : buffers_{{std::forward<DataType>(initial_back_buf)},
                   {std::forward<DataType>(initial_middle_buf)},
                   {std::forward<DataType>(initial_front_buf)}}
    {}

    class Reader
    {
    public:
        Reader(DataType& data, bool fresh) : data_(data), fresh_{fresh} {}

        const DataType& data() const noexcept { return data_; }
        bool isFresh() const noexcept { return fresh_; }

    private:
        DataType& data_;
        bool fresh_{false};
    };

    // Consumer API
    //
    // Returns reference to the latest front buffer.
    // If there's a new buffer recently written -> will swap buffers and return the latest value.
    // Otherwise, will return stale value.
    Reader read()
    {
        const auto dirty_ptr = middle_buffer_.load(std::memory_order_relaxed);
        if ((dirty_ptr & kDirtyBit) == 0) {
            return {*front_buffer_, false};
        }

        const auto prev = middle_buffer_.exchange(reinterpret_cast<std::uintptr_t>(front_buffer_), std::memory_order_acq_rel);
        front_buffer_ = reinterpret_cast<DataType*>(prev & kDirtyBitMask);
        return {*front_buffer_, true};
    }

    // Producer API
    //
    // Returns reference to back buffer, producer can use it to fill the buffer.
    // Once finished filling the buffer commit() must be called to propagate changes
    // to the front buffer.
    DataType& write() noexcept
    {
        return *back_buffer_;
    }

    // Producer API
    //
    // Propagates pending changes from the back buffer to the middle buffer.
    // The next read will get those changes unless the writer is faster and will overwrite
    // those changes before the next read() call.
    void commit()
    {
        const auto dirty_ptr = kDirtyBit | reinterpret_cast<std::uintptr_t>(back_buffer_);
        const auto prev = middle_buffer_.exchange(dirty_ptr, std::memory_order_acq_rel) & kDirtyBitMask;
        back_buffer_ = reinterpret_cast<DataType*>(prev);
    }

    // Raii scoped wrapper around write+commit for ergonomics
    class ScopedWriter
    {
    public:
        ScopedWriter(DataType& data, TripleBuffer* triple_buffer) : data_{data}, triple_buffer_{triple_buffer} {}
        ~ScopedWriter()
        {
            triple_buffer_->commit();
        }

        ScopedWriter(const ScopedWriter&) = delete;
        ScopedWriter& operator=(const ScopedWriter&) = delete;

        ScopedWriter(ScopedWriter&&) = default;
        ScopedWriter& operator=(ScopedWriter&&) = default;

        DataType& data() noexcept { return data_; }

    private:
        DataType& data_;
        TripleBuffer* triple_buffer_;
    };

    ScopedWriter getWriter() noexcept
    {
        return {write(), this};
    }

private:
    static constexpr std::size_t kNoSharing = 64;
    static constexpr std::uintptr_t kDirtyBit = 1;
    static constexpr std::uintptr_t kDirtyBitMask = ~std::uintptr_t{} ^ kDirtyBit;

    struct alignas(kNoSharing) Buffer
    {
        DataType data{};
    };

    Buffer buffers_[3];

    std::atomic<std::uintptr_t> middle_buffer_{reinterpret_cast<std::uintptr_t>(&buffers_[1].data)};
    alignas(kNoSharing) DataType* back_buffer_{&buffers_[0].data};
    alignas(kNoSharing) DataType* front_buffer_{&buffers_[2].data};
};