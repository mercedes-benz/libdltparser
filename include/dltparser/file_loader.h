// SPDX-License-Identifier: MIT
// Copyright (c) 2026 MBition GmbH

#ifndef DLTPARSER_FILE_H
#define DLTPARSER_FILE_H

#include <atomic>
#include <vector>

namespace dltp
{

    /// Single buffer for the Single Producer Single Consumer queue
    /// buffer_size is the size of the buffer in bytes
    template <std::size_t buffer_size>
    struct FileBuffer
    {

        /// Last bytes written to the buffer
        std::size_t read_bytes = 0;
        std::vector<char> buffer = std::vector<char>(buffer_size);
    };

    /// Single Producer Single Consumer queue class using a ring buffer
    /// buffer_size of the buffers in bytes
    /// N number of buffers
    template <std::size_t buffer_size, std::size_t N>
    class SPSCQueue
    {
    public:
        SPSCQueue() = default;

        /// Copy the available buffer to the queue
        /// @param item The pointer of the buffer which is pushed to this queue
        /// @return True if successful, otherwise false
        bool push(FileBuffer<buffer_size> *item);

        /// Push an available buffer from the queue to the passed address
        /// @param item The reference to the existing buffer pointer to which the buffer will be given
        /// @return True if successful, otherwise false
        bool pop(FileBuffer<buffer_size> *&item);

    private:
        static_assert((N & (N - 1)) == 0, "N must be a power of 2");

        // alignas(64) std::atomic<std::size_t> head{0}; // TODO: Check for alignment efficiency
        std::atomic<std::size_t> head{0};
        // alignas(64) std::atomic<std::size_t> tail{0}; // TODO: Check for alignment efficiency
        std::atomic<std::size_t> tail{0};
        FileBuffer<buffer_size> *buffers[N];
    };

    template <std::size_t buffer_size, std::size_t N>
    bool SPSCQueue<buffer_size, N>::push(FileBuffer<buffer_size> *item)
    {
        std::size_t t = tail.load(std::memory_order_relaxed);
        const std::size_t next = (t + 1) & (N - 1);

        if (next == head.load(std::memory_order_acquire))
        {
            return false;
        }

        buffers[t] = item;
        tail.store(next, std::memory_order_release);
        return true;
    }

    template <std::size_t buffer_size, std::size_t N>
    bool SPSCQueue<buffer_size, N>::pop(FileBuffer<buffer_size> *&item)
    {
        std::size_t h = head.load(std::memory_order_relaxed);

        if (h == tail.load(std::memory_order_acquire))
        {
            return false;
        }

        item = buffers[h];
        head.store((h + 1) & (N - 1), std::memory_order_release);
        return true;
    }

} // namespace dltp

#endif // DLTPARSER_FILE_H