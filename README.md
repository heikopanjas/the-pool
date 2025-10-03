# the-pool

A modern C++23 thread pool implementation that provides efficient task-based parallelism with support for futures, backpressure control, and graceful shutdown.

## Features

- **Fixed-size thread pool** with configurable worker count
- **Configurable task queue** with maximum size limit (default: 10,000 tasks)
- **Dual enqueueing modes**:
  - `Enqueue()` - Blocking with timeout for normal task submission
  - `TryEnqueue()` - Non-blocking for high-throughput scenarios
- **Future-based results** using `std::future` for task return values
- **Synchronization primitives** - `WaitForAllTasks()` for coordination points
- **Thread-safe operations** with proper mutex and atomic variable usage
- **RAII design** - Automatic thread cleanup on destruction
- **Backpressure handling** to prevent queue overflow

## Requirements

- C++23 compliant compiler:
  - GCC 11 or later
  - Clang 13 or later
  - MSVC 2022 or later
- CMake 3.10 or later
- pthread library (automatically linked on Unix-like systems)

## Building

### Basic Build (Static Library)

```bash
mkdir build && cd build
cmake ..
make
```

### Shared Library Build

```bash
mkdir build && cd build
cmake -DBUILD_SHARED_LIBS=ON ..
make
```

### Installation

```bash
sudo make install
```

This installs:

- Library files to `/usr/local/lib` (or system equivalent)
- Header files to `/usr/local/include`
- CMake package configuration files for easy integration

## Usage

### Basic Example

```cpp
#include "ThreadPool.h"
#include <iostream>

int main() {
    // Create a thread pool with 4 worker threads
    ThreadPool pool(4);

    // Enqueue a simple task
    auto result = pool.Enqueue([]() {
        return 42;
    });

    // Get the result
    std::cout << "Result: " << result.get() << std::endl;

    return 0;
}
```

### Enqueueing Tasks with Arguments

```cpp
#include "ThreadPool.h"
#include <iostream>

int multiply(int a, int b) {
    return a * b;
}

int main() {
    ThreadPool pool(4);

    // Enqueue a function with arguments
    auto future = pool.Enqueue(multiply, 6, 7);

    std::cout << "6 * 7 = " << future.get() << std::endl;

    return 0;
}
```

### Non-blocking Enqueue

```cpp
#include "ThreadPool.h"
#include <iostream>

int main() {
    ThreadPool pool(4, 100);  // Pool with max queue size of 100

    for (int i = 0; i < 1000; ++i) {
        if (!pool.TryEnqueue([i]() {
            std::cout << "Task " << i << std::endl;
        })) {
            std::cout << "Queue full, task " << i << " rejected" << std::endl;
            // Handle backpressure - maybe wait or log
        }
    }

    pool.WaitForAllTasks();
    return 0;
}
```

### Waiting for Task Completion

```cpp
#include "ThreadPool.h"
#include <iostream>
#include <chrono>

int main() {
    ThreadPool pool(4);

    // Enqueue multiple tasks
    for (int i = 0; i < 10; ++i) {
        pool.Enqueue([i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::cout << "Task " << i << " completed" << std::endl;
        });
    }

    // Wait for all tasks to complete before proceeding
    pool.WaitForAllTasks();

    std::cout << "All tasks completed" << std::endl;
    return 0;
}
```

## API Reference

### Constructor

```cpp
ThreadPool(size_t threads, size_t maxQueueSize = 10'000)
```

Creates a thread pool with the specified number of worker threads and maximum queue size.

**Parameters:**

- `threads` - Number of worker threads to create
- `maxQueueSize` - Maximum number of pending tasks (default: 10,000)

**Throws:**

- `std::system_error` if thread creation fails

### Enqueue

```cpp
template<class F, class... Args>
auto Enqueue(F&& f, Args&&... args) -> std::future<return_type>
```

Enqueues a task for execution. If the queue is full, waits briefly (100ms timeout) before adding the task anyway to prevent deadlock.

**Parameters:**

- `f` - Callable object (function, lambda, functor)
- `args` - Arguments to pass to the callable

**Returns:**

- `std::future` containing the result of the task

**Throws:**

- `std::runtime_error` if the thread pool has been stopped

### TryEnqueue

```cpp
template<class F, class... Args>
bool TryEnqueue(F&& f, Args&&... args)
```

Attempts to enqueue a task without blocking. Returns immediately if the queue is full.

**Parameters:**

- `f` - Callable object (function, lambda, functor)
- `args` - Arguments to pass to the callable

**Returns:**

- `true` if the task was successfully enqueued
- `false` if the queue was full or the pool was stopped

### WaitForAllTasks

```cpp
void WaitForAllTasks()
```

Blocks until all enqueued tasks have been completed and all worker threads are idle. Useful for synchronization points where all previously submitted work must finish before proceeding.

## Design Notes

### Thread Safety

All public methods are thread-safe and can be called from multiple threads concurrently. Internal synchronization is handled using:

- `std::mutex` for protecting the task queue
- `std::atomic` for frequently-accessed flags (`stop_`, `activeTasks_`)
- Three condition variables for different synchronization needs:
  - `condition_` - Worker threads waiting for tasks
  - `finished_` - Callers waiting for all tasks to complete
  - `queueNotFull_` - Producers waiting for queue space

### Backpressure Control

The thread pool implements backpressure handling to prevent unbounded queue growth:

- `Enqueue()` uses a 100ms timeout when the queue is full, then adds the task anyway to prevent deadlock while providing some backpressure
- `TryEnqueue()` returns `false` immediately if the queue is full, allowing the caller to implement custom backpressure strategies

### Performance Considerations

- **Move semantics**: Tasks are moved from the queue rather than copied
- **Lock-free execution**: Tasks execute outside of lock scope for maximum concurrency
- **Atomic counters**: Active task counter uses atomics to minimize lock contention
- **Condition variables**: Worker threads sleep when idle rather than busy-waiting

### Shutdown Behavior

The destructor:

1. Signals all worker threads to stop
2. Wakes up all waiting threads
3. Joins each worker thread to ensure clean shutdown
4. Any unprocessed tasks in the queue are discarded

## Integration with CMake Projects

After installation, you can use the thread pool in your CMake projects:

```cmake
find_package(ThreadPool REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE ThreadPool::threadpool)
```

## License

MIT License - Copyright (c) 2025 Heiko Panjas

See [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome. Please ensure:

- Code follows the existing style conventions (see [AGENTS.md](AGENTS.md))
- All public APIs have comprehensive Doxygen comments
- Thread safety considerations are documented
- Changes are tested with both shared and static library builds

## Documentation

For detailed coding standards and project conventions, see [AGENTS.md](AGENTS.md).
