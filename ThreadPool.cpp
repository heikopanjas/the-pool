#include "ThreadPool.h"
#include <algorithm>

ThreadPool::ThreadPool(size_t threads, size_t maxQueueSize) : stop_(false), activeTasks_(0), maxQueueSize_(maxQueueSize)
{
    // Create the specified number of worker threads
    for (size_t i = 0; i < threads; ++i)
    {
        // Each thread is created with a lambda that defines its work loop
        workers_.emplace_back([this] {
            // Infinite loop - will only exit when an empty task is received
            for (;;)
            {
                // Get a task from the queue - this might block if no tasks are available
                // or return an empty function if the pool is stopping
                std::function<void()> task = GetNextTask();

                // An empty task signals that the worker should exit
                // This happens when the pool is being destroyed and there are no more tasks
                if (!task)
                {
                    return;
                }

                // At this point we've removed a task from the queue, so notify any
                // producers that were waiting because the queue was full
                queueNotFull_.notify_one();

                // Execute the task - this is done outside of any locks to allow maximum concurrency
                task();

                // After task execution, update our bookkeeping and potentially notify waiters
                NotifyTaskCompletion();
            }
        });
    }
}

ThreadPool::~ThreadPool()
{
    // Tell all threads they should exit when they next check for work
    SignalThreadsToStop();

    // Wake up all threads that might be waiting on the condition variables
    // This ensures they check the stop_ flag and can exit cleanly
    condition_.notify_all();
    queueNotFull_.notify_all();

    // Wait for each thread to finish its current task and exit
    // Without this join, threads could be terminated while still working
    for (std::thread& worker : workers_)
    {
        worker.join();
    }
}

void ThreadPool::SignalThreadsToStop()
{
    // We need to hold the queue mutex while setting the stop flag
    // to ensure proper synchronization with threads that might be
    // checking this flag in their predicates
    std::unique_lock<std::mutex> lock(queueMutex_);
    stop_ = true;
}

std::function<void()> ThreadPool::GetNextTask()
{
    // Lock the queue mutex to safely access the task queue
    std::unique_lock<std::mutex> lock(queueMutex_);

    // Wait until either:
    // 1. The thread pool is being stopped (stop_ == true), or
    // 2. There's at least one task available in the queue (tasks_.empty() == false)
    // This predicate is checked whenever the condition variable is notified
    condition_.wait(lock, [this] { return stop_ || tasks_.empty() == false; });

    // If the pool is stopping AND there are no tasks left to process,
    // return an empty function to signal that the worker should exit
    if (stop_ && tasks_.empty() == true)
    {
        return {}; // Return empty function to signal exit
    }

    // At this point, we know there's at least one task in the queue
    // Move (instead of copy) the task from the queue to optimize performance
    std::function<void()> task = std::move(tasks_.front());
    tasks_.pop();

    // Increment the count of active tasks - this is used by WaitForAllTasks
    // to know when all work is completed
    activeTasks_++;

    return task;
}

void ThreadPool::NotifyTaskCompletion()
{
    // Lock the queue mutex to safely access shared state
    std::unique_lock<std::mutex> lock(queueMutex_);

    // Decrement the count of active tasks
    activeTasks_--;

    // If there are no more tasks in the queue and no tasks currently executing,
    // notify any threads that might be waiting for all work to complete
    // This is primarily used by WaitForAllTasks
    if (0 == activeTasks_ && tasks_.empty() == true)
    {
        finished_.notify_all();
    }
}

void ThreadPool::WaitForAllTasks()
{
    // Lock the queue mutex to safely access shared state
    std::unique_lock<std::mutex> lock(queueMutex_);

    // Wait until both conditions are true:
    // 1. No more tasks in the queue, and
    // 2. No tasks currently being executed by worker threads
    // The predicate is checked when the condition variable is notified
    // in NotifyTaskCompletion
    finished_.wait(lock, [this] { return tasks_.empty() && 0 == activeTasks_; });

    // When this function returns, all tasks have completed,
    // providing a synchronization point for the caller
}
