#include "log.hpp"
#include <ice/async.hpp>
#include <ice/context.hpp>
#include <thread>

namespace ice::log {
namespace {

std::atomic<std::size_t> g_limit = std::numeric_limits<std::size_t>::max();
std::atomic<std::size_t> g_count = 0;
std::shared_ptr<sink> g_sink;

class post final : public event {
public:
  post(context& context, entry entry) noexcept : context_(context), entry_(entry) {}

  constexpr bool await_ready() const noexcept
  {
    return false;
  }

  void await_suspend(std::experimental::coroutine_handle<> awaiter) noexcept
  {
    g_count.fetch_add(1, std::memory_order_release);
    event::awaiter_ = awaiter;
    context_.post(this);
  }

  void await_resume() const
  {
    g_count.fetch_sub(1, std::memory_order_release);
    if (auto sink = std::atomic_load_explicit(&g_sink, std::memory_order_acquire)) {
      sink->print(entry_);
    } else {
      print(entry_);
    }
  }

private:
  context& context_;
  entry entry_;
};

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

  template <typename T>
  T queue(log::entry entry) noexcept
  {
#if ICE_EXCEPTIONS
    try {
#endif
      co_await post{ context_, std::move(entry) };
#if ICE_EXCEPTIONS
    }
    catch (const std::exception& e) {
      ice::printf(stderr, "critical logger error: %s\n", e.what());
    }
    catch (...) {
      ice::print(stderr, "critical logger error\n");
    }
#endif
  }

private:
  std::thread thread_;
  ice::context context_;
};

logger g_logger;

}  // namespace

const char* format(level level, bool padding) noexcept
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

void queue(time_point tp, level level, ice::format format, std::string message) noexcept
{
  if (g_count.load(std::memory_order_acquire) < g_limit.load(std::memory_order_acquire)) {
    g_logger.queue<ice::task>({ tp, level, format, std::move(message) });
  } else {
    g_logger.queue<ice::sync>({ tp, level, format, std::move(message) });
  }
}

void print(const log::entry& entry)
{
  const auto stream = static_cast<int>(entry.level) > static_cast<int>(log::level::error) ? stdout : stderr;
  const auto tt = std::chrono::system_clock::to_time_t(entry.tp);
  tm tm = {};
#if ICE_OS_WIN32
  localtime_s(&tm, &tt);
#else
  localtime_r(&tt, &tm);
#endif
  const auto year = tm.tm_year + 1900;
  const auto month = tm.tm_mon + 1;
  const auto day = tm.tm_mday;
  const auto te = entry.tp.time_since_epoch();
  const auto ms = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(te).count() % 1000);
  ice::printf(stream, "%04d-%02d-%02d %02d:%02d:%02d.%03d [", year, month, day, tm.tm_hour, tm.tm_min, tm.tm_sec, ms);
  // clang-format off
  switch (entry.level) {
  case level::emergency: ice::print(stream, color::cyan,    "emergency"); break;
  case level::alert:     ice::print(stream, color::blue,    "alert    "); break;
  case level::critical:  ice::print(stream, color::magenta, "critical "); break;
  case level::error:     ice::print(stream, color::red,     "error    "); break;
  case level::warning:   ice::print(stream, color::yellow,  "warning  "); break;
  case level::notice:    ice::print(stream, color::green,   "notice   "); break;
  case level::info:      ice::print(stream,                 "info     "); break;
  case level::debug:     ice::print(stream, color::grey,    "debug    "); break;
  }
  // clang-format on
  ice::print(stream, "] ");
  if (entry.format) {
    ice::print(stream, entry.format, entry.message.data());
  } else if (entry.level == level::debug) {
    ice::print(stream, color::grey, entry.message.data());
  } else {
    ice::print(stream, entry.message.data());
  }
  ice::print(stream, '\n');
}

}  // namespace ice::log
