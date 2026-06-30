add_executable(bench_throughput bench_throughput.cpp)
target_link_libraries(bench_throughput PRIVATE spsc Threads::Threads)
target_compile_options(bench_throughput PRIVATE ${SPSC_WARN})

add_executable(bench_latency bench_latency.cpp)
target_link_libraries(bench_latency PRIVATE spsc Threads::Threads)
target_compile_options(bench_latency PRIVATE ${SPSC_WARN})
