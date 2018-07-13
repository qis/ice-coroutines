#include "common.hpp"
#include <ice/async.hpp>
#include <ice/service.hpp>
#include <thread>

#if ICE_DEBUG
constexpr std::size_t iterations = 10000;
#else
constexpr std::size_t iterations = 100000;
#endif

// Switches to the current service.
static void service_verify(benchmark::State& state) noexcept
{
  ice::service c0;
  if (const auto ec = c0.create()) {
    state.SkipWithError(ec.message().data());
    return;
  }
  [](ice::service& c0, benchmark::State& state) -> ice::task {
    const auto ose = ice::on_scope_exit([&]() { c0.stop(); });
    for (auto _ : state) {
      co_await c0.schedule();
    }
  }(c0, state);
  ice_set_thread_affinity(0);
  c0.run();
}
BENCHMARK(service_verify)->Threads(1)->Iterations(iterations);

// Switches to the current service (suspends execution).
static void service_append(benchmark::State& state) noexcept
{
  ice::service c0;
  if (const auto ec = c0.create()) {
    state.SkipWithError(ec.message().data());
    return;
  }
  [](ice::service& c0, benchmark::State& state) -> ice::task {
    const auto ose = ice::on_scope_exit([&]() { c0.stop(); });
    for (auto _ : state) {
      co_await c0.schedule(true);
    }
  }(c0, state);
  ice_set_thread_affinity(0);
  c0.run();
}
BENCHMARK(service_append)->Threads(1)->Iterations(iterations);

// Switches to the first service.
// Switches to the first service.
// Switches to the second service.
static void service_switch(benchmark::State& state) noexcept
{
  ice::service c0;
  if (const auto ec = c0.create()) {
    state.SkipWithError(ec.message().data());
    return;
  }
  ice::service c1;
  if (const auto ec = c1.create()) {
    state.SkipWithError(ec.message().data());
    return;
  }
  auto t0 = std::thread([&]() {
    ice_set_thread_affinity(0);
    c0.run();
  });
  auto t1 = std::thread([&]() {
    ice_set_thread_affinity(1);
    c1.run();
  });
  [](ice::service& c0, ice::service& c1, benchmark::State& state) -> ice::task {
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
BENCHMARK(service_switch)->Threads(1)->Iterations(iterations);

// Switches to the first service.
// Switches to the second service.
static void service_always(benchmark::State& state) noexcept
{
  ice::service c0;
  if (const auto ec = c0.create()) {
    state.SkipWithError(ec.message().data());
    return;
  }
  ice::service c1;
  if (const auto ec = c1.create()) {
    state.SkipWithError(ec.message().data());
    return;
  }
  auto t0 = std::thread([&]() {
    ice_set_thread_affinity(0);
    c0.run();
  });
  auto t1 = std::thread([&]() {
    ice_set_thread_affinity(1);
    c1.run();
  });
  [](ice::service& c0, ice::service& c1, benchmark::State& state) -> ice::task {
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
BENCHMARK(service_always)->Threads(1)->Iterations(iterations);
