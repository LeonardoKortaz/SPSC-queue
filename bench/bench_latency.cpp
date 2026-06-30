// SPSC round-trip latency benchmark (ping-pong)
#include "spsc/SpscQueue.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <vector>

using spsc::SpscQueue;
using Clock = std::chrono::steady_clock;

int main(int argc, char** argv) {
  std::uint64_t iters = 1'000'000;
  if (argc > 1) iters = std::strtoull(argv[1], nullptr, 10);

  SpscQueue<std::uint64_t> to_pong(1);
  SpscQueue<std::uint64_t> to_ping(1);

  std::thread pong([&] {
    for (std::uint64_t i = 0; i < iters; ++i) {
      to_ping.push(to_pong.pop());
    }
  });

  std::vector<double> samples;
  samples.reserve(iters);

  for (std::uint64_t i = 0; i < iters; ++i) {
    const auto t0 = Clock::now();
    to_pong.push(i);
    const std::uint64_t echoed = to_ping.pop();
    const auto t1 = Clock::now();
    if (echoed != i) {
      std::fprintf(stderr, "latency bench: token mismatch\n");
      return 1;
    }
    samples.push_back(std::chrono::duration<double, std::nano>(t1 - t0).count());
  }
  pong.join();

  std::sort(samples.begin(), samples.end());
  const auto pct = [&](double p) {
    const std::size_t idx = static_cast<std::size_t>(p * (samples.size() - 1));
    return samples[idx];
  };
  double sum = 0.0;
  for (double s : samples) sum += s;
  const double mean = sum / samples.size();

  std::printf("SPSC ping-pong latency over %llu round-trips\n",
              static_cast<unsigned long long>(iters));
  std::printf("  round-trip:  mean %8.1f ns   p50 %8.1f ns   p99 %8.1f ns   max %8.1f ns\n",
              mean, pct(0.50), pct(0.99), samples.back());
  std::printf("  one-way    : ~%7.1f ns (round-trip / 2)\n", mean / 2.0);
  return 0;
}
