// SPSC throughput benchmark: items/second for a few capacities.
// For stable numbers pin to two cores, e.g. taskset -c 2,3 ./bench_throughput

#include "spsc/SpscQueue.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <vector>

using spsc::SpscQueue;
using Clock = std::chrono::steady_clock;

static void run(std::size_t capacity, std::uint64_t n) {
  SpscQueue<std::uint64_t> q(capacity);

  const auto start = Clock::now();

  std::thread producer([&] {
    for (std::uint64_t i = 0; i < n; ++i) {
      while (!q.try_push(i)) {
      }
    }
  });

  std::uint64_t checksum = 0;
  std::uint64_t received = 0;
  while (received < n) {
    if (auto v = q.try_pop()) {
      checksum += *v;
      ++received;
    }
  }
  producer.join();

  const auto end = Clock::now();
  const double secs =
      std::chrono::duration<double>(end - start).count();
  const double million_items_per_sec = (static_cast<double>(n) / secs) / 1e6;

  const std::uint64_t expected = (n - 1) * n / 2;  // 0+1+...+(n-1)
  std::printf("capacity=%6zu  items=%llu  time=%7.3f s  throughput=%8.2f M items/s  %s\n",
              capacity, static_cast<unsigned long long>(n), secs, million_items_per_sec,
              checksum == expected ? "ok" : "CHECKSUM MISMATCH");
}

int main(int argc, char** argv) {
  std::uint64_t n = 50'000'000;
  if (argc > 1) n = std::strtoull(argv[1], nullptr, 10);

  std::printf("SPSC throughput benchmark (%llu items per run)\n",
              static_cast<unsigned long long>(n));
  for (std::size_t cap : {16u, 256u, 1024u, 8192u}) {
    run(cap, n);
  }
  return 0;
}
