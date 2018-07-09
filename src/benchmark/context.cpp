#include "common.hpp"
#include <ice/async.hpp>
#include <ice/context.hpp>
#include <thread>

#if ICE_DEBUG
constexpr std::size_t iterations = 10000;
#else
constexpr std::size_t iterations = 1000000;
#endif

// Switches to the current context.
static void context_verify(benchmark::State& state) noexcept
{
  ice::context c0;
  [](ice::context& c0, benchmark::State& state) -> ice::task {
    const auto ose = ice::on_scope_exit([&]() { c0.stop(); });
    for (auto _ : state) {
      co_await c0.schedule();
    }
  }(c0, state);
  ice_set_thread_affinity(0);
  c0.run();
}
BENCHMARK(context_verify)->Threads(1)->Iterations(iterations);

// Switches to the current context (suspends execution).
static void context_append(benchmark::State& state) noexcept
{
  ice::context c0;
  [](ice::context& c0, benchmark::State& state) -> ice::task {
    const auto ose = ice::on_scope_exit([&]() { c0.stop(); });
    for (auto _ : state) {
      co_await c0.schedule(true);
    }
  }(c0, state);
  c0.run();
}
BENCHMARK(context_append)->Threads(1)->Iterations(iterations);

// Switches to the first context.
// Switches to the first context.
// Switches to the second context.
static void context_switch(benchmark::State& state) noexcept
{
  ice::context c0;
  ice::context c1;
  auto t0 = std::thread([&]() {
    ice_set_thread_affinity(0);
    c0.run();
  });
  auto t1 = std::thread([&]() {
    ice_set_thread_affinity(1);
    c1.run();
  });
  [](ice::context& c0, ice::context& c1, benchmark::State& state) -> ice::task {
    const auto ose = ice::on_scope_exit([&]() {
      c0.stop();
      c1.stop();
    });
    auto i = 0;
    for (auto _ : state) {
      switch (i % 3) {
      case 0: [[fallthrough]];
      case 1: co_await c0.schedule(); break;
      case 2: co_await c1.schedule(); break;
      }
      i++;
    }
  }(c0, c1, state);
  t0.join();
  t1.join();
}
BENCHMARK(context_switch)->Threads(1)->Iterations(iterations);

// Switches to the first context.
// Switches to the second context.
static void context_always(benchmark::State& state) noexcept
{
  ice::context c0;
  ice::context c1;
  auto t0 = std::thread([&]() {
    ice_set_thread_affinity(0);
    c0.run();
  });
  auto t1 = std::thread([&]() {
    ice_set_thread_affinity(1);
    c1.run();
  });
  [](ice::context& c0, ice::context& c1, benchmark::State& state) -> ice::task {
    const auto ose = ice::on_scope_exit([&]() {
      c0.stop();
      c1.stop();
    });
    auto i = true;
    for (auto _ : state) {
      if (i) {
        co_await c0.schedule(true);
      } else {
        co_await c1.schedule(true);
      }
      i = !i;
    }
  }(c0, c1, state);
  t0.join();
  t1.join();
}
BENCHMARK(context_always)->Threads(1)->Iterations(iterations);
