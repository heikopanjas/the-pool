cmake_minimum_required(VERSION 3.10)
project(ThreadPool VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Library source files
set(SOURCES
    ThreadPool.cpp
)

set(HEADERS
    ThreadPool.h
)

# Create library (shared or static based on BUILD_SHARED_LIBS)
add_library(threadpool ${SOURCES} ${HEADERS})

# Create alias for FetchContent compatibility
# Allows consumers to use ThreadPool::threadpool with both find_package() and FetchContent
add_library(ThreadPool::threadpool ALIAS threadpool)

# Set include directories
target_include_directories(threadpool
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

# Link pthread library (required for std::thread)
find_package(Threads REQUIRED)
target_link_libraries(threadpool PUBLIC Threads::Threads)

# Set library properties
set_target_properties(threadpool PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "${HEADERS}"
)

# Install rules
include(GNUInstallDirs)

install(TARGETS threadpool
    EXPORT ThreadPoolTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install export targets
install(EXPORT ThreadPoolTargets
    FILE ThreadPoolTargets.cmake
    NAMESPACE ThreadPool::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ThreadPool
)

# Create and install package config file
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ThreadPoolConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/ThreadPoolConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ThreadPool
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/ThreadPoolConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/ThreadPoolConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/ThreadPoolConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ThreadPool
)
