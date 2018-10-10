#pragma once
#include <ice/config.hpp>
#include <ice/terminal.hpp>
#include <fmt/format.h>
#include <fmt/time.h>
#include <chrono>
#include <experimental/coroutine>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <cstdint>

#if ICE_EXCEPTIONS
#  include <exception>
#  include <system_error>
#endif

namespace ice::log {

using clock = std::chrono::system_clock;
using time_point = clock::time_point;

// clang-format off

enum class level : std::uint16_t {
  emergency = 0,
  alert     = 1,
  critical  = 2,
  error     = 3,
  warning   = 4,
  notice    = 5,
  info      = 6,
  debug     = 7,
  custom    = 8,
};

// clang-format on

format get_level_format(level level) noexcept;
const char* get_level_string(level level, bool padding = true) noexcept;

struct entry {
  std::tm tm;
  std::chrono::milliseconds ms;
  level level = level::info;
  format format;
  std::string message;
};

class sink {
public:
  virtual ~sink() = default;
  virtual void print(const entry& entry) = 0;
};

void set(std::shared_ptr<sink> sink);

void limit(std::size_t queue_size) noexcept;

void queue(time_point tp, level level, format format, std::string message) noexcept;

template <typename Arg, typename... Args>
inline void queue(time_point tp, level level, format format, fmt::string_view message, Arg&& arg, Args&&... args)
{
  queue(tp, level, format, fmt::format(message, std::forward<Arg>(arg), std::forward<Args>(args)...));
}

inline void queue(time_point tp, level level, std::string message) noexcept
{
  queue(tp, level, format{}, std::move(message));
}

template <typename Arg, typename... Args>
inline void queue(time_point tp, level level, fmt::string_view message, Arg&& arg, Args&&... args)
{
  queue(tp, level, format{}, fmt::format(message, std::forward<Arg>(arg), std::forward<Args>(args)...));
}

// clang-format off

template <typename Arg, typename... Args>
inline int queue(time_point tp, level level, std::error_code ec, format format, fmt::string_view message, Arg&& arg, Args&&... args)
{
  queue(tp, level, format,
    fmt::format("{} error {}: {} ({})", ec.category().name(), ec.value(),
      fmt::format(message, std::forward<Arg>(arg), std::forward<Args>(args)...), ec.message()));
  return ec.value();
}

inline int queue(time_point tp, level level, std::error_code ec, std::string message) noexcept
{
  queue(tp, level, format{},
    fmt::format("{} error {}: {} ({})", ec.category().name(), ec.value(), message, ec.message()));
  return ec.value();
}

template <typename Arg, typename... Args>
inline int queue(time_point tp, level level, std::error_code ec, fmt::string_view message, Arg&& arg, Args&&... args)
{
  queue(tp, level, format{},
    fmt::format("{} error {}: {} ({})", ec.category().name(), ec.value(),
      fmt::format(message, std::forward<Arg>(arg), std::forward<Args>(args)...), ec.message()));
  return ec.value();
}

// clang-format on

template <typename... Args>
inline auto emergency(Args&&... args)
{
  return queue(clock::now(), level::emergency, std::forward<Args>(args)...);
}

template <typename... Args>
inline auto alert(Args&&... args)
{
  return queue(clock::now(), level::alert, std::forward<Args>(args)...);
}

template <typename... Args>
inline auto critical(Args&&... args)
{
  return queue(clock::now(), level::critical, std::forward<Args>(args)...);
}

template <typename... Args>
inline auto error(Args&&... args)
{
  return queue(clock::now(), level::error, std::forward<Args>(args)...);
}

template <typename... Args>
inline auto warning(Args&&... args)
{
  return queue(clock::now(), level::warning, std::forward<Args>(args)...);
}

template <typename... Args>
inline auto notice(Args&&... args)
{
  return queue(clock::now(), level::notice, std::forward<Args>(args)...);
}

template <typename... Args>
inline auto info(Args&&... args)
{
  return queue(clock::now(), level::info, std::forward<Args>(args)...);
}

template <typename... Args>
inline auto debug(Args&&... args)
{
  return queue(clock::now(), level::debug, std::forward<Args>(args)...);
}

template <typename... Args>
inline auto custom(Args&&... args)
{
  return queue(clock::now(), level::custom, std::forward<Args>(args)...);
}

void print(const entry& entry);

void stop() noexcept;

struct task {
  struct promise_type {
    constexpr task get_return_object() const noexcept
    {
      return {};
    }

    constexpr auto initial_suspend() const noexcept
    {
      return std::experimental::suspend_never{};
    }

    constexpr auto final_suspend() const noexcept
    {
      return std::experimental::suspend_never{};
    }

    constexpr void return_void() const noexcept {}

#if ICE_EXCEPTIONS || defined(__clang__)
    void unhandled_exception() const noexcept(ICE_NO_EXCEPTIONS)
    {
#  if ICE_EXCEPTIONS
      try {
        throw;
      }
      catch (const std::system_error& e) {
        const auto ec = e.code();
        error("{} error {}: {} ({})", ec.category().name(), ec.value(), e.what(), ec.message());
      }
      catch (const std::exception& e) {
        error(e.what());
      }
      catch (...) {
        error("unhandled exception");
      }
#  endif
    }
#endif
  };
};

}  // namespace ice::log
