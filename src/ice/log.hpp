#pragma once
#include <ice/config.hpp>
#include <ice/print.hpp>
#include <chrono>
#include <memory>
#include <string>
#include <system_error>
#include <cstdint>

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
};

// clang-format on

const char* format(level level, bool padding = true) noexcept;

struct entry {
  log::time_point tp;
  log::level level = log::level::info;
  ice::format format;
  std::string message;
};

class sink {
public:
  virtual ~sink() = default;
  virtual void print(const ice::log::entry& entry) = 0;
};

void set(std::shared_ptr<sink> sink);

void limit(std::size_t queue_size) noexcept;

// clang-format off

void queue(time_point tp, level level, ice::format format, std::string message) noexcept;

template <typename Arg, typename... Args>
inline void queue(time_point tp, level level, ice::format format, fmt::string_view message, Arg&& arg, Args&&... args)
{
  queue(tp, level, format, fmt::format(message, std::forward<Arg>(arg), std::forward<Args>(args)...));
}

inline void queue(time_point tp, level level, std::string message) noexcept
{
  queue(tp, level, ice::format{}, std::move(message));
}

template <typename Arg, typename... Args>
inline void queue(time_point tp, level level, fmt::string_view message, Arg&& arg, Args&&... args)
{
  queue(tp, level, fmt::format(message, std::forward<Arg>(arg), std::forward<Args>(args)...));
}

inline void queue(time_point tp, level level, ice::format format, std::error_code ec, std::string message) noexcept
{
  queue(tp, level, format, fmt::format("{} error {}: {} ({})", ec.category().name(), ec.value(), message, ec.message()));
}

template <typename Arg, typename... Args>
inline void queue(time_point tp, level level, ice::format format, std::error_code ec, fmt::string_view message, Arg&& arg, Args&&... args)
{
  queue(tp, level, format, ec, fmt::format(message, std::forward<Arg>(arg), std::forward<Args>(args)...));
}

inline void queue(time_point tp, level level, std::error_code ec, std::string message) noexcept
{
  queue(tp, level, ice::format{}, ec, std::move(message));
}

template <typename Arg, typename... Args>
inline void queue(time_point tp, level level, std::error_code ec, fmt::string_view message, Arg&& arg, Args&&... args)
{
  queue(tp, level, ec, fmt::format(message, std::forward<Arg>(arg), std::forward<Args>(args)...));
}

inline void queue(time_point tp, level level, ice::format format, std::error_code ec) noexcept
{
  queue(tp, level, format, fmt::format("{} error {}: {}", ec.category().name(), ec.value(), ec.message()));
}

inline void queue(time_point tp, level level, std::error_code ec) noexcept
{
  queue(tp, level, ice::format{}, ec);
}

// clang-format on

template <typename... Args>
inline void emergency(Args&&... args)
{
  queue(clock::now(), level::emergency, std::forward<Args>(args)...);
}

template <typename... Args>
inline void alert(Args&&... args)
{
  queue(clock::now(), level::alert, std::forward<Args>(args)...);
}

template <typename... Args>
inline void critical(Args&&... args)
{
  queue(clock::now(), level::critical, std::forward<Args>(args)...);
}

template <typename... Args>
inline void error(Args&&... args)
{
  queue(clock::now(), level::error, std::forward<Args>(args)...);
}

template <typename... Args>
inline void warning(Args&&... args)
{
  queue(clock::now(), level::warning, std::forward<Args>(args)...);
}

template <typename... Args>
inline void notice(Args&&... args)
{
  queue(clock::now(), level::notice, std::forward<Args>(args)...);
}

template <typename... Args>
inline void info(Args&&... args)
{
  queue(clock::now(), level::info, std::forward<Args>(args)...);
}

template <typename... Args>
inline void debug(Args&&... args)
{
  queue(clock::now(), level::debug, std::forward<Args>(args)...);
}

void print(const log::entry& entry);

}  // namespace ice::log
