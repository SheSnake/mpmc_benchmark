#include "mpmc_lock_free_queue.hpp"
#include "mpmc_atomic_queue.hpp"
#include "mpmc_mutex_queue.hpp"
#include <iostream>
#include <thread>
#include <stdio.h>
#include <queue>
#include <chrono>

void sigle_thread_test(int size)
{
    mpmc_queue::cas::MpmcBoundedQueue<int> mpmc_q(1 << 20, true);
    std::queue<int> std_q;
    std::int64_t t0 = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    for (int i = 0; i < size; ++i)
    {
        std_q.push(i);
        std_q.pop();
    }
    std::int64_t t1 = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    int t;
    for (int i = 0; i < size; ++i)
    {
        mpmc_q.enqueue_t(i);
        mpmc_q.dequeue_t(t);
    }
    std::int64_t t2 = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    printf("push %d element, std_q [%ld]us, mpmc_q [%ld]us\n", size, t1 - t0, t2 - t1);
}

template <typename T, typename Queue>
void producer(Queue* mpmc_q, int size)
{
    std::int64_t t0 = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    for (int i = 0; i < size; ++i)
    {
        int ret = mpmc_q->enqueue_t(T(i));
        while (ret < 0)
        {
            ret = mpmc_q->enqueue_t(T(i));
            //printf("queue full\n");
        }
    }
    std::int64_t t1 = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    printf("produce %d element, mpmc_q [%ld]us\n", size, t1 - t0);
}

template <typename T, typename Queue>
void consumer(Queue* mpmc_q, int size)
{
    std::int64_t t0 = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    for (int i = 0; i < size; ++i)
    {
        T t;
        int ret = mpmc_q->dequeue_t(t);
        if (ret < 0)
        {
            printf("queue empty\n");
        }
    }
    std::int64_t t1 = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    printf("consume %d element, mpmc_q [%ld]us\n", size, t1 - t0);
}


template <typename T, typename Queue>
void multi_thread_test(Queue* mpmc_q, int thread_num, int size)
{
    std::thread* producers = new std::thread[thread_num];
    std::thread* consumers = new std::thread[thread_num];
    for (int i = 0; i < thread_num; ++i)
    {
        int per_size = size / thread_num;
        producers[i] = std::thread(producer<T, Queue>, mpmc_q, per_size);
    }
    for (int i = 0; i < thread_num; ++i)
    {
        int per_size = size / thread_num;
        consumers[i] = std::thread(consumer<T, Queue>, mpmc_q, per_size);
    }
    for (int i = 0; i < thread_num; ++i)
    {
        producers[i].join();
        consumers[i].join();
    }
    delete [] producers;
    delete [] consumers;
}

template <typename T>
void multi_thread_test(int size, int thread_num)
{
    printf("begin cas queue test\n");
    mpmc_queue::cas::MpmcBoundedQueue<T> mpmc_cas_q(1 << 15, true);
    multi_thread_test<T, mpmc_queue::cas::MpmcBoundedQueue<T>>(&mpmc_cas_q, thread_num, size);
    
    printf("\nbegin mutex + std::queue test\n");
    mpmc_queue::mut::MpmcBoundedQueue<T> mpmc_mut_q(true);
    multi_thread_test<T, mpmc_queue::mut::MpmcBoundedQueue<T>>(&mpmc_mut_q, thread_num, size);

    mpmc_queue::ato::MpmcBoundedQueue<T> mpmc_ato_q(true);
    printf("\nbegin atomic exchange + std::queue test\n");
    multi_thread_test<T, mpmc_queue::ato::MpmcBoundedQueue<T>>(&mpmc_ato_q, thread_num, size);
}

template<int size=1>
struct Data
{
    int t[size];
    Data()
    {
        for (int i = 0; i < size; ++i)
        {
            t[i] = i;
        }
    }
    Data(int m)
    {
        for (int i = 0; i < size; ++i)
        {
            t[i] = m;
        }
    }
};

int main()
{
    //sigle_thread_test(1000000);
    multi_thread_test<Data<1000>>(100000, 2);
}
