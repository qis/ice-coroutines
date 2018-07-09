#pragma once
#include <ice/config.hpp>
#include <ice/terminal.hpp>
#include <fmt/format.h>
#include <fmt/time.h>
#include <chrono>
#include <memory>
#include <string>
#include <string_view>
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

ice::format get_level_format(level level) noexcept;
const char* get_level_string(level level, bool padding = true) noexcept;

struct entry {
  std::tm tm;
  std::chrono::milliseconds ms;
  ice::log::level level = level::info;
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

void queue(time_point tp, level level, ice::format format, std::string message) noexcept;

template <typename Arg, typename... Args>
inline void queue(time_point tp, level level, ice::format format, std::string_view message, Arg&& arg, Args&&... args)
{
  queue(tp, level, format, fmt::format(message, std::forward<Arg>(arg), std::forward<Args>(args)...));
}

inline void queue(clock::time_point tp, level level, std::string message) noexcept
{
  queue(tp, level, ice::format{}, std::move(message));
}

template <typename Arg, typename... Args>
inline void queue(time_point tp, level level, std::string_view message, Arg&& arg, Args&&... args)
{
  queue(tp, level, fmt::format(message, std::forward<Arg>(arg), std::forward<Args>(args)...));
}

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

void print(const entry& entry);

}  // namespace ice::log
