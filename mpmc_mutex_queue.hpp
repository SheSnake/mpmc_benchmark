#pragma once
#include <atomic>
#include <queue>
#include <mutex>

namespace mpmc_queue
{

namespace mut
{

template <typename T>
class MpmcBoundedQueue
{
    std::mutex mut_;
    std::queue<T> q_;
    bool block_;
public:
    MpmcBoundedQueue(bool block)
        : block_(block)
    {}

    int enqueue_t(const T& data)
    {
        std::unique_lock<std::mutex> lock(mut_);
        q_.push(data);
        return 0;
    }

    int dequeue_t(T& data)
    {
        bool fetch = false;
        while (!fetch)
        {
            std::unique_lock<std::mutex> lock(mut_);
            if (q_.empty())
            {
                if (block_)
                {
                    continue;
                }
                else
                {
                    return -1;
                }
            }
            data = q_.front();
            q_.pop();
            fetch = true;
        }
        return 0;
    }
};

}; // ns mut


};
