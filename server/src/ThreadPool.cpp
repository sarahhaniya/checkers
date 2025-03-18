// server/src/ThreadPool.cpp
#include "../include/ThreadPool.h"

ThreadPool::ThreadPool(size_t numThreads) : stop(false)
{
    for (size_t i = 0; i < numThreads; ++i)
    {
        workers.emplace_back([this]
                             {
            while (true) {
                std::function<void()> task;
                
                {
                    std::unique_lock<std::mutex> lock(this->queueMutex);
                    
                    // Wait until we have a task or the pool is stopped
                    this->condition.wait(lock, [this] {
                        return this->stop || !this->tasks.empty();
                    });
                    
                    // If pool is stopped and no tasks, exit thread
                    if (this->stop && this->tasks.empty()) {
                        return;
                    }
                    
                    // Get task from queue
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }
                
                // Execute task
                task();
            } });
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop = true;
    }

    // Wake up all threads
    condition.notify_all();

    // Join all threads
    for (std::thread &worker : workers)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }
}