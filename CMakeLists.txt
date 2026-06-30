cmake_minimum_required(VERSION 3.14)
project(spsc_queue LANGUAGES CXX)

add_library(spsc INTERFACE)
target_include_directories(spsc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(spsc INTERFACE cxx_std_17)
