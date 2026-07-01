// Concurrent stress tests: one producer, one consumer. Run under ThreadSanitizer for a race check.

#include "spsc/SpscQueue.hpp"

#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

using spsc::SpscQueue;

TEST(SpscConcurrent, NonBlockingPreservesOrderAndCount) {
  constexpr std::uint64_t N = 5'000'000;
  SpscQueue<std::uint64_t> q(16);

  std::thread producer([&] {
    for (std::uint64_t i = 0; i < N; ++i) {
      while (!q.try_push(i)) {
        std::this_thread::yield();
      }
    }
  });

  std::uint64_t expected = 0;
  std::uint64_t received = 0;
  while (received < N) {
    auto v = q.try_pop();
    if (!v) {
      std::this_thread::yield();
      continue;
    }
    ASSERT_EQ(*v, expected) << "FIFO order violated at item " << received;
    ++expected;
    ++received;
  }

  producer.join();
  EXPECT_EQ(received, N);
  EXPECT_TRUE(q.empty());
}

TEST(SpscConcurrent, BlockingApiPreservesOrderAndCount) {
  constexpr std::uint64_t N = 2'000'000;
  SpscQueue<std::uint64_t> q(64);

  std::thread producer([&] {
    for (std::uint64_t i = 0; i < N; ++i) q.push(i);
  });

  for (std::uint64_t i = 0; i < N; ++i) {
    std::uint64_t v = q.pop();
    ASSERT_EQ(v, i) << "FIFO order violated at item " << i;
  }

  producer.join();
  EXPECT_TRUE(q.empty());
}

TEST(SpscConcurrent, MoveOnlyPayload) {
  constexpr std::uint64_t N = 1'000'000;
  SpscQueue<std::unique_ptr<std::uint64_t>> q(32);

  std::thread producer([&] {
    for (std::uint64_t i = 0; i < N; ++i) {
      q.push(std::make_unique<std::uint64_t>(i));
    }
  });

  for (std::uint64_t i = 0; i < N; ++i) {
    std::unique_ptr<std::uint64_t> v = q.pop();
    ASSERT_NE(v, nullptr);
    ASSERT_EQ(*v, i);
  }

  producer.join();
  EXPECT_TRUE(q.empty());
}

TEST(SpscConcurrent, ChecksumIntegrity) {
  constexpr std::uint64_t N = 3'000'000;
  SpscQueue<std::uint64_t> q(128);

  std::uint64_t produced_sum = 0;
  std::thread producer([&] {
    for (std::uint64_t i = 1; i <= N; ++i) {
      produced_sum += i;
      q.push(i);
    }
  });

  std::uint64_t consumed_sum = 0;
  for (std::uint64_t i = 0; i < N; ++i) consumed_sum += q.pop();

  producer.join();
  EXPECT_EQ(produced_sum, consumed_sum);
}
