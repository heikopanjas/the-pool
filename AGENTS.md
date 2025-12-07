# Project Instructions for AI Coding Agents

**Last updated:** 2025-12-07

## Project Overview

**the-pool** is a modern C++ thread pool implementation that provides efficient task-based parallelism. It manages a fixed-size pool of worker threads that execute tasks asynchronously with support for futures, backpressure control, and graceful shutdown.

### Key Features

- Fixed-size thread pool with configurable worker count
- Task queue with configurable maximum size (default: 10,000)
- Support for both blocking (`Enqueue`) and non-blocking (`TryEnqueue`) task submission
- Future-based task results using `std::future`
- Synchronization primitives for waiting on task completion (`WaitForAllTasks`)
- Thread-safe operations with proper RAII cleanup
- Backpressure handling to prevent queue overflow

## Technology Stack

- **Language**: C++23
- **Standard Library**: STL threading primitives
  - `std::thread`, `std::mutex`, `std::condition_variable`
  - `std::future`, `std::packaged_task`
  - `std::atomic`, `std::function`
- **Build System**: CMake 3.10+
- **License**: MIT License (2025)

## Coding Conventions

### Code Formatting

The project uses `.clang-format` for automatic code formatting. Key formatting rules include:

- **Indentation**: 4 spaces (no tabs)
- **Column Limit**: 160 characters
- **Brace Style**: Custom with specific wrapping rules (see `.clang-format`)
- **Pointer/Reference Alignment**: Left (`int* ptr`, `int& ref`)
- **Access Modifier Offset**: -4 (align with class keyword)
- **Line Endings**: LF (Unix-style)

**Important:** Always run clang-format before committing code to ensure consistency.

### C++ Style

- **Header Guards**: Use `#ifndef __FILENAME_H_INCL__` pattern with double underscores
- **Naming**:
  - PascalCase for class names (e.g., `ThreadPool`)
  - PascalCase for public methods (e.g., `Enqueue`, `WaitForAllTasks`)
  - camelCase with trailing underscore for private members (e.g., `stop_`, `workers_`)
  - PascalCase for private methods (e.g., `SignalThreadsToStop`, `GetNextTask`)
- **Template Placement**: Template implementations in header file (required for templates)
- **Documentation**: Doxygen-style comments with `///` and `\brief`, `\param`, `\return`, `\note`, `\throws` tags
- **Comparisons**: Constant on left-hand side of equality comparisons (e.g., `true == stop_`, `0 == activeTasks_`)
- **Alignment**: Consecutive assignments, declarations, and macros are aligned (enforced by clang-format)
- **Empty Lines**: Maximum 1 empty line between code blocks
- **Function Parameters**: All input parameters should be const (e.g., `void Foo(const int value)`)

### Design Patterns

- **RAII**: Constructor creates threads, destructor joins them
- **Non-copyable**: Delete copy constructor and assignment operator
- **Lock Management**: Use `std::unique_lock<std::mutex>` for automatic unlock
- **Condition Variables**: Clear predicates in lambda form for `wait()` calls
- **Move Semantics**: Prefer `std::move()` for task extraction from queue

### Thread Safety

- All public methods are thread-safe
- Mutex protection for shared state (`queueMutex_`)
- Atomic variables for frequently-accessed flags (`stop_`, `activeTasks_`)
- Three condition variables for different synchronization needs:
  - `condition_`: Worker threads waiting for tasks
  - `finished_`: Callers waiting for all tasks to complete
  - `queueNotFull_`: Producers waiting for queue space

### Error Handling

- Throw `std::runtime_error` for invalid operations (e.g., enqueue on stopped pool)
- Document potential exceptions with `\throws` in comments
- Document system errors (e.g., thread creation failures)

## Build Commands

Build system is CMake-based with Ninja generator. Standard workflow:

```bash
# Configure (static library by default)
mkdir build && cd build
cmake -G Ninja ..

# Configure for shared library
cmake -G Ninja -DBUILD_SHARED_LIBS=ON ..

# Build
cmake --build .

# Install (optional)
cmake --install . --prefix /usr/local

# Create packages
cmake --build . --target package_source  # Source archives
cmake --build . --target package         # Binary packages
```

Compiler requirements: C++23 support required (GCC 11+, Clang 13+, MSVC 2022+)

## CI/CD - GitHub Actions

The project uses GitHub Actions for continuous integration and release automation.

### Build Workflow (`.github/workflows/build.yml`)

**Triggers:**
- Push to `develop` branch
- Push to `feature/**` branches
- Pull requests targeting `develop` or `feature/**` branches

**Actions:**
- Builds library on Linux (Ubuntu 22.04), macOS, and Windows
- Tests multiple compilers:
  - Linux: GCC 11, GCC 12, Clang 14
  - macOS: Apple Clang (latest)
  - Windows: MSVC 2022
- Creates CPack packages for each platform
- Uploads packages as workflow artifacts

**Purpose:** Validates that code changes compile successfully across platforms and compilers before merging.

### Release Workflow (`.github/workflows/release.yml`)

**Triggers:**
- Push of tag matching `v*` pattern (e.g., `v1.0.0`)
- Tag must point to a commit on the `main` branch

**Actions:**
- Verifies tag is on `main` branch (fails early if not)
- Builds library on Linux, macOS, and Windows
- Creates platform-specific packages:
  - Linux: `.tar.gz`, `.deb`, `.rpm`
  - macOS: `.tar.gz`
  - Windows: `.zip`
- Creates source archives (`.tar.gz` and `.zip`)
- Creates GitHub Release with:
  - All generated packages attached
  - Auto-generated release notes with FetchContent usage instructions

**Release creation workflow:**
```bash
# Ensure you're on main branch
git checkout main
git pull origin main

# Create and push tag
git tag -a v1.0.0 -m "Release version 1.0.0"
git push origin v1.0.0

# GitHub Actions will automatically create the release
```

## Best Practices

### Task Enqueueing

- Use `Enqueue()` for normal task submission (blocks briefly if queue full)
- Use `TryEnqueue()` for non-blocking submission in high-throughput scenarios
- Always check the returned future if you need the task result
- Handle `std::runtime_error` when enqueueing might occur after shutdown

### Queue Management

- Default queue size (10,000) is suitable for most scenarios
- Adjust `maxQueueSize` in constructor for memory-constrained or high-throughput applications
- `Enqueue()` uses 100ms timeout when queue is full to balance backpressure vs. deadlock prevention

### Synchronization

- Use `WaitForAllTasks()` at synchronization points where all work must complete
- Be aware that `WaitForAllTasks()` blocks until queue is empty AND all active tasks finish
- The destructor automatically waits for threads to finish, no manual cleanup needed

### Performance Considerations

- Worker threads use move semantics to extract tasks from queue
- Tasks execute outside of lock scope for maximum concurrency
- Active task counter uses atomics to minimize lock contention
- Condition variables minimize busy-waiting

### Code Documentation

- Maintain comprehensive Doxygen comments for all public APIs
- Explain synchronization logic in implementation comments
- Document why certain patterns are used (e.g., "prevents deadlock")
- Include thread safety notes in method documentation

## Working with AI Coding Agents

### Instruction File Management

This project uses a two-file instruction system:

1. **`AGENTS.md`** (this file) - Primary instructions for AI coding agents
2. **`.github/copilot-instructions.md`** - Simple pointer file that references AGENTS.md

### Update Protocol

**When to update AGENTS.md:**

- Coding standards, conventions, or project decisions evolve
- New patterns or best practices are established
- Technology stack changes
- Build system or tooling updates

**Update requirements:**

- Maintain the "Last updated" timestamp at the top
- Add entries to the "Recent Updates & Decisions" log at the bottom with:
  - Date (YYYY-MM-DD format)
  - Brief description of changes
  - Reasoning for the change
- Preserve the document structure: title header → timestamp → main instructions → "Recent Updates & Decisions" section

**Do NOT modify `.github/copilot-instructions.md`** unless the reference mechanism itself needs changes.

### Commit Workflow

When committing changes:

1. Stage the changes using git commands
2. Write detailed but concise commit messages using [Conventional Commits](https://www.conventionalcommits.org/) format
3. **CRITICAL: NEVER commit automatically** - always wait for explicit confirmation from the user
4. Present the staged changes and commit message for review before proceeding

---

## Recent Updates & Decisions

### 2025-10-03 - Initial Documentation

- Created AGENTS.md based on existing ThreadPool implementation
- Documented current C++ coding conventions observed in codebase
- Established documentation standards for future development

### 2025-10-03 - C++23 and CMake Integration

- Updated project to C++23 standard (from C++11/14)
- Added CMakeLists.txt with shared/static library support via BUILD_SHARED_LIBS
- Updated build commands with CMake workflow
- Documented compiler requirements (GCC 11+, Clang 13+, MSVC 2022+)

### 2025-10-03 - Comparison Convention Terminology

- Updated terminology from "Yoda conditions" to "constant on left-hand side of equality comparisons"
- This convention prevents accidental assignment in conditionals and improves code safety

### 2025-10-03 - Added clang-format Configuration

- Added `.clang-format` file with project-specific formatting rules
- Key settings: 4-space indentation, 160 column limit, custom brace wrapping, left pointer alignment
- Updated AGENTS.md to reference clang-format as the authoritative formatting source
- Emphasized importance of running clang-format before commits

### 2025-10-03 - Const Function Parameters

- Updated all function input parameters to be const
- Added convention to coding guidelines: all input parameters should be const
- Improves code safety by preventing accidental modification of parameters
- Template forwarding references (T&&) remain unchanged as they require perfect forwarding

### 2025-12-07 - FetchContent Compatibility

- Added alias target `ThreadPool::threadpool` for FetchContent compatibility
- Consumers can now use the same target name with both `find_package()` and `FetchContent`
- This follows CMake best practices for library distribution

### 2025-12-07 - Build System Enhancements

- Updated documentation to use Ninja generator and generator-agnostic cmake --build commands
- Added CMake-configured version header (ThreadPoolVersion.h.in)
- Added CPack configuration for source and binary package generation
- Added file headers with copyright and license information to all source files

### 2025-12-07 - GitHub Actions CI/CD

- Added build workflow for develop and feature branches with multi-compiler testing
- Added release workflow for automated GitHub releases on v* tags
- Release workflow validates that tags are on main branch before proceeding
- Automated package creation for Linux, macOS, and Windows platforms

