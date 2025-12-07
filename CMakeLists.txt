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

# Configure version header
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/ThreadPoolVersion.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/ThreadPoolVersion.h
    @ONLY
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
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
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

# Install generated version header
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ThreadPoolVersion.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
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

# CPack configuration for package generation
set(CPACK_PACKAGE_NAME "ThreadPool")
set(CPACK_PACKAGE_VENDOR "Heiko Panjas")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Modern C++23 thread pool library with futures and backpressure control")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_CONTACT "heikopanjas@github")

# Source package configuration
set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-src")
set(CPACK_SOURCE_IGNORE_FILES
    /.git/
    /.github/
    /build/
    /Build/
    /.cursor/
    /.vscode/
    /.idea/
    /.vs/
    /.DS_Store
    /CMakeLists.txt.user
    /.clangd/
    /.cache/
)

# Binary package generators (platform-specific)
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

if(UNIX AND NOT APPLE)
    # Linux: DEB and RPM packages
    list(APPEND CPACK_GENERATOR "TGZ" "DEB" "RPM")

    # Debian package configuration
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Heiko Panjas")
    set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libstdc++6")

    # RPM package configuration
    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
    set(CPACK_RPM_PACKAGE_GROUP "Development/Libraries")
    set(CPACK_RPM_PACKAGE_REQUIRES "glibc, libstdc++")
elseif(APPLE)
    # macOS: TGZ archive
    list(APPEND CPACK_GENERATOR "TGZ")
elseif(WIN32)
    # Windows: ZIP archive and NSIS installer
    list(APPEND CPACK_GENERATOR "ZIP")
endif()

include(CPack)
