#include "ice/log.hpp"
#include <ice/async.hpp>
#include <ice/context.hpp>
#include <thread>

namespace ice::log {
namespace {

std::atomic<std::size_t> g_limit = std::numeric_limits<std::size_t>::max();
std::atomic<std::size_t> g_count = 0;
std::shared_ptr<sink> g_sink;

class logger {
public:
  logger()
  {
    thread_ = std::thread([this]() noexcept { context_.run(); });
  }

  ~logger()
  {
    context_.stop();
    if (thread_.joinable()) {
      thread_.join();
    }
  }

  task queue(log::entry entry) noexcept
  {
    g_count.fetch_add(1, std::memory_order_release);
#if ICE_EXCEPTIONS
    try {
#endif
      co_await context_.schedule(true);
      if (auto sink = std::atomic_load_explicit(&g_sink, std::memory_order_acquire)) {
        sink->print(entry);
      } else {
        print(entry);
      }
#if ICE_EXCEPTIONS
    }
    catch (const std::exception& e) {
      std::fprintf(stderr, "critical logger error: %s\n", e.what());
    }
    catch (...) {
      std::fputs("critical logger error\n", stderr);
    }
#endif
    g_count.fetch_sub(1, std::memory_order_release);
  }

  task queue(const log::entry& entry, std::atomic_bool& completed, std::condition_variable& cv) noexcept
  {
#if ICE_EXCEPTIONS
    try {
#endif
      co_await context_.schedule(true);
      if (auto sink = std::atomic_load_explicit(&g_sink, std::memory_order_acquire)) {
        sink->print(entry);
      } else {
        print(entry);
      }
#if ICE_EXCEPTIONS
    }
    catch (const std::exception& e) {
      std::fprintf(stderr, "critical logger error: %s\n", e.what());
    }
    catch (...) {
      std::fputs("critical logger error\n", stderr);
    }
#endif
    completed.store(true, std::memory_order_release);
    cv.notify_one();
  }

private:
  std::thread thread_;
  ice::context context_;
};

logger g_logger;

}  // namespace

ice::format get_level_format(level level) noexcept
{
  // clang-format off
  switch (level) {
  case level::emergency: return color::cyan;
  case level::alert:     return color::blue;
  case level::critical:  return color::magenta;
  case level::error:     return color::red;
  case level::warning:   return color::yellow;
  case level::notice:    return color::green;
  case level::info:      return {};
  case level::debug:     return color::grey;
  }
  // clang-format on
  return {};
}

const char* get_level_string(level level, bool padding) noexcept
{
  // clang-format off
  switch (level) {
  case level::emergency: return padding ? "emergency" : "emergency";
  case level::alert:     return padding ? "alert    " : "alert";
  case level::critical:  return padding ? "critical " : "critical";
  case level::error:     return padding ? "error    " : "error";
  case level::warning:   return padding ? "warning  " : "warning";
  case level::notice:    return padding ? "notice   " : "notice";
  case level::info:      return padding ? "info     " : "info";
  case level::debug:     return padding ? "debug    " : "debug";
  }
  // clang-format on
  return padding ? "unknown  " : "unknown";
}

void set(std::shared_ptr<sink> sink)
{
  std::atomic_store_explicit(&g_sink, sink, std::memory_order_release);
}

void limit(std::size_t queue_size) noexcept
{
  g_limit.store(queue_size, std::memory_order_release);
}

void queue(time_point tp, level level, format format, std::string message) noexcept
{
  const auto tt = std::chrono::system_clock::to_time_t(tp);
  tm tm = {};
#if ICE_OS_WIN32
  localtime_s(&tm, &tt);
#else
  localtime_r(&tt, &tm);
#endif
  const auto te = tp.time_since_epoch();
  const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(te).count() % 1000;
  if (g_count.load(std::memory_order_acquire) < g_limit.load(std::memory_order_acquire)) {
    g_logger.queue({ tm, std::chrono::milliseconds{ ms }, level, format, std::move(message) });
  } else {
    std::atomic_bool completed = false;
    std::condition_variable cv;
    std::mutex mutex;
    std::unique_lock lock{ mutex };
    g_logger.queue({ tm, std::chrono::milliseconds{ ms }, level, format, std::move(message) }, completed, cv);
    cv.wait(lock, [&]() { return completed.load(std::memory_order_acquire); });
  }
}

void print(const log::entry& entry)
{
  const auto stream = static_cast<int>(entry.level) > static_cast<int>(log::level::error) ? stdout : stderr;
  const auto level_format = get_level_format(entry.level);
  const auto level_string = get_level_string(entry.level);
  fmt::print(stream, "{:%Y-%m-%d %H:%M:%S}.{:#03d} [", entry.tm, entry.ms.count());
  if (terminal::is_tty(stream)) {
    terminal::manager manager{ stream, level_format };
    std::fputs(level_string, stream);
    manager.reset();
    std::fputs("] ", stream);
    if (entry.format) {
      manager.set(entry.format);
    } else if (entry.level == level::debug) {
      manager.set(level_format);
    }
    std::fputs(entry.message.data(), stream);
    manager.reset();
    std::fputc('\n', stream);
  } else {
    fmt::print(stream, "{}] {}\n", level_string, entry.message);
  }
}

}  // namespace ice::log
