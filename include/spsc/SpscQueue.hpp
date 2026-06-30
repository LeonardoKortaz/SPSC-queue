// Bounded single-producer / single-consumer queue.

#ifndef SPSC_QUEUE_HPP
#define SPSC_QUEUE_HPP

#include <atomic>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace spsc {

template <class T>
class SpscQueue {
 public:
  explicit SpscQueue(std::size_t capacity)
      : capacity_(capacity), buffer_(capacity + 1) {
    if (capacity == 0) {
      throw std::invalid_argument("capacity must be >= 1");
    }
  }

  SpscQueue(const SpscQueue&) = delete;
  SpscQueue& operator=(const SpscQueue&) = delete;

  bool try_push(const T& item) {
    const std::size_t tail = tail_.load(std::memory_order_relaxed);
    const std::size_t next_tail = next(tail);
    if (next_tail == cached_head_) {
      cached_head_ = head_.load(std::memory_order_acquire);
      if (next_tail == cached_head_) return false;
    }
    buffer_[tail] = item;
    tail_.store(next_tail, std::memory_order_release);
    return true;
  }

  bool try_push(T&& item) {
    const std::size_t tail = tail_.load(std::memory_order_relaxed);
    const std::size_t next_tail = next(tail);
    if (next_tail == cached_head_) {
      cached_head_ = head_.load(std::memory_order_acquire);
      if (next_tail == cached_head_) return false;
    }
    buffer_[tail] = std::move(item);
    tail_.store(next_tail, std::memory_order_release);
    return true;
  }

  std::optional<T> try_pop() {
    const std::size_t head = head_.load(std::memory_order_relaxed);
    if (head == cached_tail_) {
      cached_tail_ = tail_.load(std::memory_order_acquire);
      if (head == cached_tail_) return std::nullopt;
    }
    std::optional<T> result(std::move(buffer_[head]));
    head_.store(next(head), std::memory_order_release);
    return result;
  }

  void push(const T& item) {
    while (!try_push(item)) std::this_thread::yield();
  }
  void push(T&& item) {
    while (!try_push(std::move(item))) std::this_thread::yield();
  }
  T pop() {
    for (;;) {
      if (auto item = try_pop()) return std::move(*item);
      std::this_thread::yield();
    }
  }

  void clear() {
    while (try_pop().has_value()) {
    }
  }

  bool empty() const {
    return head_.load(std::memory_order_acquire) ==
           tail_.load(std::memory_order_acquire);
  }
  bool full() const {
    const std::size_t tail = tail_.load(std::memory_order_acquire);
    return next(tail) == head_.load(std::memory_order_acquire);
  }
  std::size_t capacity() const { return capacity_; }

  std::size_t size() const {
    const std::size_t tail = tail_.load(std::memory_order_acquire);
    const std::size_t head = head_.load(std::memory_order_acquire);
    return tail >= head ? tail - head : buffer_.size() - head + tail;
  }

 private:
  std::size_t next(std::size_t i) const noexcept {
    ++i;
    return i == buffer_.size() ? 0 : i;
  }

  std::size_t capacity_;
  std::vector<T> buffer_;
  alignas(64) std::atomic<std::size_t> head_{0};
  alignas(64) std::atomic<std::size_t> tail_{0};
  alignas(64) std::size_t cached_head_ = 0;
  alignas(64) std::size_t cached_tail_ = 0;
  
};
}

#endif
