#pragma once
#include <atomic>
#include <queue>

namespace mpmc_queue
{

namespace ato
{

template <typename T>
class MpmcBoundedQueue
{
    std::atomic<bool> lock_;
    std::queue<T> q_;
    bool block_;
public:
    MpmcBoundedQueue(bool block)
        : block_(block)
    {
        lock_ = false;
    }
    int enqueue_t(const T& data)
    {
        while (lock_.exchange(true));
        q_.push(data);
        lock_.store(false, std::memory_order_seq_cst);
        return 0;
    }

    int dequeue_t(T& data)
    {
        bool fetch = false;
        while (!fetch)
        {
            while (lock_.exchange(true));
            if (q_.empty())
            {
                lock_.store(false);
                if (block_)
                {
                    continue;
                }
                return -1;
            }
            data = q_.front();
            q_.pop();
            fetch = true;
            lock_.store(false);
        }
        return 0;
    }
};

}; // ns ato


};
