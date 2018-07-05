#include <ice/async.hpp>
#include <ice/context.hpp>
#include <ice/utility.hpp>
#include <benchmark/benchmark.h>
#include <thread>

#if ICE_DEBUG
constexpr std::size_t iterations = 10000;
#else
constexpr std::size_t iterations = 1000000;
#endif

// Switches to the current context.
static void context_verify(benchmark::State& state) noexcept
{
  static ice::context c0;
  [&]() -> ice::task {
    for (auto _ : state) {
      co_await c0.schedule();
    }
    c0.stop();
  }();
  if (const auto ec = ice::set_thread_affinity(0)) {
    state.SkipWithError(ec.message().data());
  }
  c0.run();
}
BENCHMARK(context_verify)->Threads(1)->Iterations(iterations);

// Switches to the current context (suspends execution).
static void context_append(benchmark::State& state) noexcept
{
  static ice::context c0;
  [&]() -> ice::task {
    for (auto _ : state) {
      co_await c0.schedule(true);
    }
    c0.stop();
  }();
  c0.run();
}
BENCHMARK(context_append)->Threads(1)->Iterations(iterations);

// Switches to the first context.
// Switches to the first context.
// Switches to the second context.
static void context_switch(benchmark::State& state) noexcept
{
  static ice::context c0;
  static ice::context c1;
  auto t0 = std::thread([&]() {
    ice::set_thread_affinity(0);
    c0.run();
  });
  auto t1 = std::thread([&]() {
    ice::set_thread_affinity(1);
    c1.run();
  });
  [&]() -> ice::task {
    auto i = 0;
    for (auto _ : state) {
      switch (i % 3) {
      case 0: [[fallthrough]];
      case 1: co_await c0.schedule(); break;
      case 2: co_await c1.schedule(); break;
      }
      i++;
    }
    c0.stop();
    c1.stop();
  }();
  t0.join();
  t1.join();
}
BENCHMARK(context_switch)->Threads(1)->Iterations(iterations);

// Switches to the first context.
// Switches to the second context.
static void context_always(benchmark::State& state) noexcept
{
  static ice::context c0;
  static ice::context c1;
  auto t0 = std::thread([&]() {
    ice::set_thread_affinity(0);
    c0.run();
  });
  auto t1 = std::thread([&]() {
    ice::set_thread_affinity(1);
    c1.run();
  });
  [&]() -> ice::task {
    auto i = true;
    for (auto _ : state) {
      if (i) {
        co_await c0.schedule(true);
      } else {
        co_await c1.schedule(true);
      }
      i = !i;
    }
    c0.stop();
    c1.stop();
  }();
  t0.join();
  t1.join();
}
BENCHMARK(context_always)->Threads(1)->Iterations(iterations);
