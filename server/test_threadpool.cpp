// server/test_threadpool.cpp
#include "include/ThreadPool.h"
#include <iostream>
#include <chrono>
#include <mutex>

std::mutex coutMutex;

void printNumber(int n)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate work

    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "Thread " << std::this_thread::get_id()
                  << " processed task: " << n << std::endl;
    }
}

int main()
{
    // Create thread pool with 4 worker threads
    ThreadPool pool(4);

    // Enqueue 20 tasks
    for (int i = 0; i < 20; ++i)
    {
        pool.enqueue([i]
                     { printNumber(i); });
    }

    std::cout << "All tasks enqueued" << std::endl;

    // Wait for a moment to allow tasks to complete
    std::this_thread::sleep_for(std::chrono::seconds(3));

    std::cout << "Main thread exiting" << std::endl;
    return 0;
}