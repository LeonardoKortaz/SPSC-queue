// Single-threaded correctness tests.

#include "spsc/SpscQueue.hpp"

#include <atomic>
#include <memory>
#include <string>

#include <gtest/gtest.h>

using spsc::SpscQueue;

TEST(SpscBasic, RejectsZeroCapacity) {
  EXPECT_THROW(SpscQueue<int>(0), std::invalid_argument);
}

TEST(SpscBasic, StartsEmpty) {
  SpscQueue<int> q(4);
  EXPECT_TRUE(q.empty());
  EXPECT_FALSE(q.full());
  EXPECT_EQ(q.capacity(), 4u);
  EXPECT_EQ(q.size(), 0u);
  EXPECT_FALSE(q.try_pop().has_value());
}

TEST(SpscBasic, PushThenPopSingle) {
  SpscQueue<int> q(4);
  EXPECT_TRUE(q.try_push(42));
  EXPECT_FALSE(q.empty());
  EXPECT_EQ(q.size(), 1u);

  auto v = q.try_pop();
  ASSERT_TRUE(v.has_value());
  EXPECT_EQ(*v, 42);
  EXPECT_TRUE(q.empty());
}

TEST(SpscBasic, FifoOrdering) {
  SpscQueue<int> q(8);
  for (int i = 0; i < 5; ++i) ASSERT_TRUE(q.try_push(i));
  for (int i = 0; i < 5; ++i) {
    auto v = q.try_pop();
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, i);
  }
}

TEST(SpscBasic, FillReportsFullAndRejects) {
  SpscQueue<int> q(3);
  EXPECT_TRUE(q.try_push(1));
  EXPECT_TRUE(q.try_push(2));
  EXPECT_TRUE(q.try_push(3));
  EXPECT_TRUE(q.full());
  EXPECT_EQ(q.size(), 3u);
  EXPECT_FALSE(q.try_push(4));
  EXPECT_FALSE(q.try_push(5));

  EXPECT_EQ(*q.try_pop(), 1);
  EXPECT_EQ(*q.try_pop(), 2);
  EXPECT_EQ(*q.try_pop(), 3);
  EXPECT_FALSE(q.try_pop().has_value());
}

TEST(SpscBasic, CapacityOneEdgeCase) {
  SpscQueue<int> q(1);
  EXPECT_TRUE(q.try_push(7));
  EXPECT_TRUE(q.full());
  EXPECT_FALSE(q.try_push(8));
  EXPECT_EQ(*q.try_pop(), 7);
  EXPECT_TRUE(q.empty());
  EXPECT_TRUE(q.try_push(9));
  EXPECT_EQ(*q.try_pop(), 9);
}

TEST(SpscBasic, WrapAroundManyCycles) {
  SpscQueue<int> q(4);
  int next_in = 0;
  int next_out = 0;
  for (int cycle = 0; cycle < 1000; ++cycle) {
    while (q.try_push(next_in)) ++next_in;
    while (true) {
      auto v = q.try_pop();
      if (!v) break;
      EXPECT_EQ(*v, next_out);
      ++next_out;
    }
  }
  EXPECT_EQ(next_in, next_out);
}

TEST(SpscBasic, PairType) {
  SpscQueue<std::pair<int, std::string>> q(4);
  EXPECT_TRUE(q.try_push({1, "one"}));
  auto v = q.try_pop();
  ASSERT_TRUE(v.has_value());
  EXPECT_EQ(v->first, 1);
  EXPECT_EQ(v->second, "one");
}

TEST(SpscBasic, NonTrivialType) {
  SpscQueue<std::string> q(4);
  EXPECT_TRUE(q.try_push(std::string("hello")));
  std::string longish(1000, 'x');
  EXPECT_TRUE(q.try_push(longish));
  EXPECT_EQ(*q.try_pop(), "hello");
  EXPECT_EQ(*q.try_pop(), longish);
}

TEST(SpscBasic, MoveOnlyType) {
  SpscQueue<std::unique_ptr<int>> q(4);
  EXPECT_TRUE(q.try_push(std::make_unique<int>(123)));
  auto v = q.try_pop();
  ASSERT_TRUE(v.has_value());
  ASSERT_NE(*v, nullptr);
  EXPECT_EQ(**v, 123);
}

struct Counted {
  static std::atomic<int> live;
  int value;
  explicit Counted(int v = 0) : value(v) { live.fetch_add(1); }
  Counted(const Counted& o) : value(o.value) { live.fetch_add(1); }
  Counted(Counted&& o) noexcept : value(o.value) { live.fetch_add(1); }
  Counted& operator=(const Counted&) = default;
  Counted& operator=(Counted&&) noexcept = default;
  ~Counted() { live.fetch_sub(1); }
};
std::atomic<int> Counted::live{0};

TEST(SpscBasic, DestructorDestroysRemainingElements) {
  ASSERT_EQ(Counted::live.load(), 0);
  {
    SpscQueue<Counted> q(8);
    for (int i = 0; i < 5; ++i) ASSERT_TRUE(q.try_push(Counted(i)));
    EXPECT_EQ(q.try_pop()->value, 0);
    EXPECT_EQ(q.try_pop()->value, 1);
    EXPECT_GT(Counted::live.load(), 0);
  }
  EXPECT_EQ(Counted::live.load(), 0);
}
