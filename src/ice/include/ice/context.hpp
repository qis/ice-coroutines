#pragma once
#include <ice/config.hpp>
#include <ice/scheduler.hpp>
#include <ice/utility.hpp>
#include <condition_variable>
#include <mutex>

namespace ice {

class context final : public scheduler<context> {
public:
  void run() noexcept
  {
    const auto index = index_.set(this);
    std::unique_lock lock{ mutex_ };
    lock.unlock();
    while (true) {
      lock.lock();
      auto head = acquire();
      while (!head) {
        if (stop_.load(std::memory_order_acquire)) {
          lock.unlock();
          return;
        }
        cv_.wait(lock, []() { return true; });
        head = acquire();
      }
      lock.unlock();
      while (head) {
        auto next = head->next.load(std::memory_order_relaxed);
        head->resume();
        head = next;
      }
    }
  }

  bool is_current() const noexcept
  {
    return index_.get() ? true : false;
  }

  void stop() noexcept
  {
    stop_.store(true, std::memory_order_release);
    cv_.notify_all();
  }

  void post(ice::schedule<context>* schedule) noexcept
  {
    scheduler::post(schedule);
    cv_.notify_one();
  }

private:
  std::atomic_bool stop_ = false;
  thread_local_storage index_;
  std::condition_variable cv_;
  std::mutex mutex_;
};

}  // namespace ice
