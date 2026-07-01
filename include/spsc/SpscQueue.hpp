// Bounded single-producer / single-consumer queue.

#ifndef SPSC_QUEUE_HPP
#define SPSC_QUEUE_HPP

#include <atomic>
#include <cstddef>
#include <memory>
#include <new>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>

namespace spsc {

inline constexpr std::size_t kCacheLineSize = 64;

template <class T>
class SpscQueue {
 public:
  // one spare slot: lets empty (head==tail) and full (next(tail)==head) stay distinct
  explicit SpscQueue(std::size_t capacity)
      : capacity_(require_positive(capacity)),
        slot_count_(capacity + 1),
        base_(allocate(capacity + 1 + 2 * kPadElems)),
        buffer_(base_ + kPadElems) {}

  ~SpscQueue() {
    std::size_t head = head_.load(std::memory_order_relaxed);
    const std::size_t tail = tail_.load(std::memory_order_relaxed);
    while (head != tail) {
      slot(head)->~T();
      head = next(head);
    }
    deallocate(base_, slot_count_ + 2 * kPadElems);
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
    new (slot(tail)) T(item);
    tail_.store(next_tail, std::memory_order_release);  // release: pairs with the consumer's acquire on tail_
    return true;
  }

  bool try_push(T&& item) {
    const std::size_t tail = tail_.load(std::memory_order_relaxed);
    const std::size_t next_tail = next(tail);
    if (next_tail == cached_head_) {
      cached_head_ = head_.load(std::memory_order_acquire);
      if (next_tail == cached_head_) return false;
    }
    new (slot(tail)) T(std::move(item));
    tail_.store(next_tail, std::memory_order_release);
    return true;
  }

  std::optional<T> try_pop() {
    const std::size_t head = head_.load(std::memory_order_relaxed);
    if (head == cached_tail_) {
      cached_tail_ = tail_.load(std::memory_order_acquire);
      if (head == cached_tail_) return std::nullopt;
    }
    T* cell = slot(head);
    std::optional<T> result(std::move(*cell));
    cell->~T();
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


  std::size_t capacity() const { return capacity_; }


   // these three are approximate while both threads are running
  bool empty() const {
    return head_.load(std::memory_order_acquire) ==
           tail_.load(std::memory_order_acquire);
  }
  bool full() const {
    const std::size_t tail = tail_.load(std::memory_order_acquire);
    return next(tail) == head_.load(std::memory_order_acquire);
  }


  std::size_t size() const {
    const std::size_t tail = tail_.load(std::memory_order_acquire);
    const std::size_t head = head_.load(std::memory_order_acquire);
    return tail >= head ? tail - head : slot_count_ - head + tail;
  }


 private:
  static constexpr std::size_t kPadElems =
      (kCacheLineSize + sizeof(T) - 1) / sizeof(T);

  static std::size_t require_positive(std::size_t capacity) {
    if (capacity == 0) {
      throw std::invalid_argument("capacity must be >= 1");
    }
    return capacity;
  }

  static T* allocate(std::size_t count) {
    return std::allocator<T>().allocate(count);
  }
  static void deallocate(T* p, std::size_t count) noexcept {
    std::allocator<T>().deallocate(p, count);
  }

  T* slot(std::size_t i) noexcept { return buffer_ + i; }

  std::size_t next(std::size_t i) const noexcept {
    ++i;
    return i == slot_count_ ? 0 : i;
  }

  const std::size_t capacity_;
  const std::size_t slot_count_; 
  T* const base_;
  T* const buffer_;
  alignas(kCacheLineSize) std::atomic<std::size_t> head_{0};
  alignas(kCacheLineSize) std::atomic<std::size_t> tail_{0};
  alignas(kCacheLineSize) std::size_t cached_head_ = 0;
  alignas(kCacheLineSize) std::size_t cached_tail_ = 0;
};
}

#endif
