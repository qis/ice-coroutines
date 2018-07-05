#pragma once
#include <ice/config.hpp>
#include <atomic>
#include <experimental/coroutine>
#include <cassert>

namespace ice {

class event {
public:
  event() noexcept = default;

  // clang-format off
#ifdef __INTELLISENSE__
  event(event&& other) {}
  event(const event& other) {}
  event& operator=(event&& other) { return *this; }
  event& operator=(const event& other) { return *this; }
#else
  event(event&& other) = delete;
  event(const event& other) = delete;
  event& operator=(event&& other) = delete;
  event& operator=(const event& other) = delete;
#endif
  // clang-format on

  virtual ~event() = default;

  virtual void resume() noexcept
  {
    awaiter_.resume();
  }

  std::atomic<event*> next = nullptr;

protected:
  std::experimental::coroutine_handle<> awaiter_;
};

class scheduler {
public:
  scheduler() = default;

  scheduler(scheduler&& other) = delete;
  scheduler(const scheduler& other) = delete;
  scheduler& operator=(scheduler&& other) = delete;
  scheduler& operator=(const scheduler& other) = delete;

  virtual ~scheduler() = default;

protected:
  event* acquire(bool ordered = true) noexcept
  {
    if (ordered) {
      auto head = head_.exchange(nullptr, std::memory_order_acquire);
      if (!head || !head->next.load(std::memory_order_relaxed)) {
        return head;
      }
      event* prev = nullptr;
      event* next = nullptr;
      while (head) {
        next = head->next.exchange(prev, std::memory_order_relaxed);
        prev = head;
        head = next;
      }
      return prev;
    }
    return head_.exchange(nullptr, std::memory_order_acquire);
  }

  void post(event* ev) noexcept
  {
    assert(ev);
    auto head = head_.load(std::memory_order_acquire);
    do {
      ev->next.store(head, std::memory_order_relaxed);
    } while (!head_.compare_exchange_weak(head, ev, std::memory_order_release, std::memory_order_acquire));
  }

  void process() noexcept
  {
    auto head = acquire(true);
    while (head) {
      auto next = head->next.load(std::memory_order_relaxed);
      head->resume();
      head = next;
    }
  }

private:
  std::atomic<event*> head_ = nullptr;
};

template <typename Scheduler>
class schedule final : public event {
public:
  static_assert(std::is_base_of_v<scheduler, Scheduler>);

  schedule(Scheduler& scheduler, bool post = false) noexcept :
    scheduler_(scheduler), ready_(!post && scheduler.is_current())
  {}

  constexpr bool await_ready() const noexcept
  {
    return ready_;
  }

  void await_suspend(std::experimental::coroutine_handle<> awaiter) noexcept
  {
    event::awaiter_ = awaiter;
    scheduler_.post(this);
  }

  constexpr void await_resume() const noexcept {}

private:
  Scheduler& scheduler_;
  const bool ready_ = true;
};

}  // namespace ice
