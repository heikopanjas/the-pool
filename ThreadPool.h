#ifndef __THREAD_POOL_H_INCL__
#define __THREAD_POOL_H_INCL__

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <vector>

///
/// \brief Thread pool that manages a collection of worker threads
///
/// The ThreadPool class provides a fixed-size pool of threads that can be used
/// to execute tasks asynchronously. It supports enqueueing tasks with arguments,
/// waiting for task completion, and controlled shutdown.
///
/// \note This class is not copyable or movable.
/// \note Thread safety: All operations are thread-safe.
///
class ThreadPool
{
public:
    ///
    /// \brief Constructs a thread pool with the specified number of threads
    ///
    /// \param threadCount Number of worker threads to create in the pool
    /// \param maxQueueSize Maximum size of the task queue (defaults to 10'000)
    /// \throws std::system_error If thread creation fails
    ///
    ThreadPool(const size_t threadCount, const size_t maxQueueSize = 10'000);

    ///
    /// \brief Destructor - stops all threads and waits for their completion
    ///
    /// The destructor signals all threads to stop and then joins each thread.
    /// It is safe to call the destructor even if tasks are still in progress.
    /// Any unprocessed tasks will not be executed.
    ///
    ~ThreadPool();

    // Delete copy constructor and assignment operator
    ThreadPool(const ThreadPool&)            = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    ///
    /// \brief Enqueues a task to be executed by the thread pool
    ///
    /// This method accepts a callable object and its arguments, then wraps
    /// it in a task that will be executed by one of the worker threads.
    /// If the queue is full, this method will block briefly and then add the task
    /// regardless of queue size to prevent deadlock.
    ///
    /// \tparam F Type of the callable object
    /// \tparam Args Types of arguments to pass to the callable object
    /// \param f The callable object to execute
    /// \param args Arguments to pass to the callable object
    /// \return std::future<return_type> A future that will hold the result of the task
    /// \throws std::runtime_error If the thread pool has been stopped
    ///
    template<class F, class... Args> auto Enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type>;

    ///
    /// \brief Attempts to enqueue a task without blocking
    ///
    /// Non-blocking version of Enqueue that returns false if the queue is full
    /// rather than waiting. This is useful for implementing backpressure in
    /// high-throughput scenarios.
    ///
    /// \tparam F Type of the callable object
    /// \tparam Args Types of arguments to pass to the callable object
    /// \param f The callable object to execute
    /// \param args Arguments to pass to the callable object
    /// \return bool True if the task was enqueued, false if the queue was full or the pool was stopped
    ///
    template<class F, class... Args> bool TryEnqueue(F&& f, Args&&... args);

    ///
    /// \brief Blocks until all tasks are completed
    ///
    /// This method waits until both the task queue is empty and all currently
    /// executing tasks have completed. It's useful for synchronization points
    /// where all previously submitted work must be finished before proceeding.
    ///
    void WaitForAllTasks();

private:
    std::vector<std::thread>          workers_;      ///< Collection of worker threads
    std::queue<std::function<void()>> tasks_;        ///< Queue of pending tasks
    std::mutex                        queueMutex_;   ///< Mutex protecting the task queue
    std::condition_variable           condition_;    ///< Condition variable for task availability
    std::condition_variable           finished_;     ///< Condition variable for task completion
    std::condition_variable           queueNotFull_; ///< Condition variable for queue space
    std::atomic<bool>                 stop_;         ///< Flag indicating shutdown
    std::atomic<size_t>               activeTasks_;  ///< Counter of currently executing tasks
    const size_t                      maxQueueSize_; ///< Maximum number of pending tasks

    ///
    /// \brief Signals all worker threads to stop processing
    ///
    /// This method sets the stop flag to true while holding the queue mutex,
    /// which will cause worker threads to exit their processing loop once
    /// they check the condition variable. It is called by the destructor.
    ///
    /// \note Thread safety: Must be called with queueMutex_ locked
    ///
    void SignalThreadsToStop();

    ///
    /// \brief Retrieves the next task from the queue
    ///
    /// This method handles the synchronization for worker threads waiting for
    /// new tasks. It either returns the next task or an empty function if the
    /// worker should exit (when the pool is stopping and the queue is empty).
    ///
    /// \return std::function<void()> Task function to be executed or empty function if worker should exit
    /// \note Thread safety: Acquires and releases the queueMutex_
    /// \note Blocks until a task is available or the pool is stopping
    ///
    std::function<void()> GetNextTask();

    ///
    /// \brief Notifies that a task has been completed
    ///
    /// This method decrements the active task counter and notifies waiting threads
    /// if all tasks have completed. It is called by worker threads after executing
    /// a task.
    ///
    /// \note Thread safety: Acquires and releases the queueMutex_
    ///
    void NotifyTaskCompletion();
};

///
/// \brief Template implementation of Enqueue method - must be in header
///
/// \note This implementation uses a packaged_task to handle the execution
///       and capturing of the callable's result in a future object.
///
template<class F, class... Args> auto ThreadPool::Enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type>
{
    using return_type = typename std::invoke_result<F, Args...>::type;

    // Create a packaged task that binds the function and args
    auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<return_type>     futureResult = task->get_future();
    std::unique_lock<std::mutex> lock(queueMutex_);

    // Don't allow enqueueing after stopping the pool
    if (true == stop_)
    {
        throw std::runtime_error("enqueue on stopped ThreadPool");
    }

    // If queue is full, wait only briefly to avoid deadlock
    if (tasks_.size() >= maxQueueSize_)
    {
        auto timeout = std::chrono::milliseconds(100);
        if (false == queueNotFull_.wait_for(lock, timeout, [this] { return stop_ || tasks_.size() < maxQueueSize_; }))
        {
            // Timeout - queue still full, but don't block indefinitely
            // This prevents deadlock while still providing some backpressure
        }
    }

    // Add the task to the queue and notify one waiting worker
    tasks_.emplace([task]() { (*task)(); });

    condition_.notify_one();
    return futureResult;
}

///
/// \brief Non-blocking enqueue implementation - template must be in header
///
/// \note This implementation differs from Enqueue by never waiting when the
///       queue is full, instead returning false immediately. Useful for
///       implementing backpressure mechanisms.
///
template<class F, class... Args> bool ThreadPool::TryEnqueue(F&& f, Args&&... args)
{
    std::unique_lock<std::mutex> lock(queueMutex_);

    // Don't allow enqueueing after stopping the pool
    if (true == stop_)
    {
        return false;
    }

    // If queue is full, return false immediately
    if (tasks_.size() >= maxQueueSize_)
    {
        return false;
    }

    // Create and add packaged task
    auto task = std::make_shared<std::packaged_task<void()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    tasks_.emplace([task]() { (*task)(); });

    // Notify one worker thread that a task is available
    condition_.notify_one();
    return true;
}

#endif // __THREAD_POOL_H_INCL__
