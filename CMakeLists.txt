cmake_minimum_required(VERSION 3.14)
project(spsc_queue LANGUAGES CXX)

add_library(spsc INTERFACE)
target_include_directories(spsc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(spsc INTERFACE cxx_std_17)

# Tests need threads (the queue is exercised from two threads).
find_package(Threads REQUIRED)

if(MSVC)
  set(SPSC_WARN /W4)
else()
  set(SPSC_WARN -Wall -Wextra -Wpedantic)
endif()

enable_testing()
add_subdirectory(tests)
