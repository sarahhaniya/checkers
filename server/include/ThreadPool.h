// server/include/ThreadPool.h
#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <stdexcept>

class ThreadPool
{
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queueMutex;
    std::condition_variable condition;
    std::atomic<bool> stop;

public:
    ThreadPool(size_t numThreads);
    ~ThreadPool();

    // Add task to the thread pool
    template <class F>
    void enqueue(F &&f)
    {
        {
            std::unique_lock<std::mutex> lock(queueMutex);

            // Don't allow enqueueing after stopping the pool
            if (stop)
            {
                throw std::runtime_error("Enqueue on stopped ThreadPool");
            }

            tasks.emplace(std::forward<F>(f));
        }
        condition.notify_one();
    }
};

#endif // THREAD_POOL_H