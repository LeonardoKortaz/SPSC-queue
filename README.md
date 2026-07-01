# SPSC queue (C++17)

A bounded single-producer / single-consumer queue - a lock-free ring buffer
for passing values from one thread to exactly one other thread.

The only dependency is GoogleTest, fetched automatically just for the tests.

Implementation: `include/spsc/SpscQueue.hpp`.

```cpp
spsc::SpscQueue<int> q(1234);
q.try_push(12);                  // returns false if full
if (auto x = q.try_pop()) use(*x);   // try_pop returns std::optional<T>
q.push(12);  int v = q.pop();    // blocking variants
```

## Assumptions / known limitations

- Exactly one producer thread and one consumer thread. More than one of either isn't
  supported.
- Capacity is fixed at construction (>= 1).
- `T` has to be movable; no default constructor needed.
- `empty()` / `full()` / `size()` are approximate while both threads are running -
  exact only when the queue is idle.
- The blocking `push`/`pop` busy-wait, trading CPU for latency.

## How to build

CMake >= 3.14 and a C++17 compiler (gcc or clang).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## How to run the tests

```bash
ctest --test-dir build --output-on-failure
```

You can run them under ThreadSanitizer as a race check:

```bash
cmake -S . -B build-tsan -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_FLAGS="-fsanitize=thread -g"
cmake --build build-tsan -j && ctest --test-dir build-tsan --output-on-failure
```

There are also two benchmarks under `bench/`:
`./build/bench/bench_throughput` and `./build/bench/bench_latency`.
