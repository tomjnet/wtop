cmake_minimum_required(VERSION 3.20)

project(wtop
  VERSION 0.1.0
  DESCRIPTION "A Windows Task Manager style top for the terminal"
  LANGUAGES CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "wtop only supports Linux (it reads /proc and /sys).")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(WTOP_BUILD_TESTS "Build the wtop unit tests" ON)

find_package(Threads REQUIRED)

add_library(wtop_core STATIC
  src/app.cc
  src/collectors/cpu_collector.cc
  src/collectors/disk_collector.cc
  src/collectors/gpu_collector.cc
  src/collectors/memory_collector.cc
  src/collectors/network_collector.cc
  src/collectors/proc_parsers.cc
  src/core/file_util.cc
  src/core/format.cc
  src/core/pci_ids.cc
  src/terminal/input.cc
  src/terminal/screen_buffer.cc
  src/terminal/terminal.cc
  src/ui/function_bar.cc
  src/ui/overlay.cc
  src/ui/performance_pane.cc
  src/ui/sidebar.cc
  src/ui/status_line.cc
  src/ui/tab_bar.cc
  src/ui/views/cpu_view.cc
  src/ui/views/disk_view.cc
  src/ui/views/gpu_view.cc
  src/ui/views/memory_view.cc
  src/ui/views/network_view.cc
  src/ui/widgets.cc
)
target_include_directories(wtop_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(wtop_core PUBLIC -Wall -Wextra -Wpedantic)
target_compile_definitions(wtop_core PUBLIC
  WTOP_VERSION="${PROJECT_VERSION}")
target_link_libraries(wtop_core PUBLIC Threads::Threads)

add_executable(wtop src/main.cc)
target_link_libraries(wtop PRIVATE wtop_core)

# Release binaries are linked fully statically (against musl on Alpine) so a
# single file runs on any Linux distribution of the same architecture.
option(WTOP_STATIC "Link the wtop executable fully statically" OFF)
if(WTOP_STATIC)
  target_link_options(wtop PRIVATE -static)
endif()

# The man page carries the project version, so it is generated at configure
# time: `man ./build/wtop.1` before installing, `man wtop` after.
include(GNUInstallDirs)
configure_file(docs/wtop.1.in ${CMAKE_CURRENT_BINARY_DIR}/wtop.1 @ONLY)

install(TARGETS wtop RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/wtop.1
  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

if(WTOP_BUILD_TESTS)
  enable_testing()

  # Use an installed GoogleTest when available, otherwise fetch a pinned
  # release. GoogleTest is only needed for the tests, never by wtop itself.
  find_package(GTest 1.14 CONFIG QUIET)
  if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(googletest
      URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
      URL_HASH SHA256=65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
  endif()

  add_executable(wtop_tests
    tests/format_test.cc
    tests/input_test.cc
    tests/parsers_test.cc
    tests/ring_buffer_test.cc
    tests/screen_buffer_test.cc
  )
  target_link_libraries(wtop_tests PRIVATE wtop_core GTest::gtest_main)
  include(GoogleTest)
  gtest_discover_tests(wtop_tests)
endif()
