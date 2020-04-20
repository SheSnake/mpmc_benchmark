#pragma once
#include <atomic>

namespace mpmc_queue
{

namespace cas
{

template <typename T>
class MpmcBoundedQueue
{
    std::atomic<std::size_t> enqueue_pos_;
    std::atomic<std::size_t> dequeue_pos_;
    std::size_t mask_; // to implement ring buffer
    bool block_;
    struct Cell
    {
        T data;
        std::atomic<std::size_t> seq_;
    };
    Cell *data_;
public:
    MpmcBoundedQueue(std::size_t buffer_size, bool block)
        : block_(block)
    {
        // make sure buffer size if pow(2, n) and > 2
        if (((buffer_size & (buffer_size - 1)) == 0) && buffer_size > 2)
        {
            mask_ = buffer_size - 1;
        }
        else
        {
            mask_ = 1 << 10; // default size 1024
        }
        enqueue_pos_ = 0;
        dequeue_pos_ = 0;
        data_ = new Cell[buffer_size];
        for (int i = 0; i < buffer_size; ++i)
        {
            data_[i].seq_ = i;
        }
    }
    int enqueue_t(const T& data)
    {
        std::size_t pos = enqueue_pos_.load();
        while (true)
        {
            std::size_t cell_seq = data_[pos & mask_].seq_.load();
            intptr_t diff = (intptr_t)cell_seq - (intptr_t)pos;
            if (diff > 0)
            {
                // this cell is used by other thread
                pos = enqueue_pos_.load();
                continue;
            }
            else if (diff == 0)
            {
                // this cell has not been used, try to occupy it
                if (enqueue_pos_.compare_exchange_strong(pos, pos + 1))
                {
                    break;
                }
            }
            else
            {
                // queue is full
                return -1;
            }
        }
        data_[pos & mask_].data = data;
        data_[pos & mask_].seq_.fetch_add(1);
        return 0;
    }

    int dequeue_t(T& data)
    {
        std::size_t pos = dequeue_pos_.load();
        while (true)
        {
            std::size_t cell_seq = data_[pos & mask_].seq_.load();
            intptr_t diff = (intptr_t)cell_seq - (intptr_t)pos - 1;
            if (diff < 0)
            {
                // empty
                if (!block_)
                {
                    return -1;
                }
            }
            else if (diff == 0)
            {
                // try to access this pos
                if (dequeue_pos_.compare_exchange_weak(pos, pos + 1))
                {
                    break;
                }
                
            }
            else
            {
                // this pos is already accessed
                pos = dequeue_pos_.load();
            }
        }
        data = data_[pos & mask_].seq_.load();
        data_[pos & mask_].seq_.fetch_add(mask_);
        return 0;
    }
};

}; // ns cas

}; // ns mpmc_queue
