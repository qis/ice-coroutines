#pragma once
#include <ice/config.hpp>
#include <fmt/format.h>
#include <cstdint>
#include <cstdio>

namespace ice {

// clang-format off

enum class color : std::uint8_t {
  none      = 0,
  grey      = 1,
  red       = 2,
  green     = 3,
  yellow    = 4,
  blue      = 5,
  magenta   = 6,
  cyan      = 7,
  white     = 8,
};

enum class style : std::uint16_t {
  none      = 0x0,
  bold      = 0x1  << 4,
  dark      = 0x2  << 4,
  underline = 0x4  << 4,
  blink     = 0x8  << 4,
  reverse   = 0x10 << 4,
  conceal   = 0x20 << 4,
};

// clang-format on

class format {
public:
  constexpr format() noexcept = default;
  constexpr explicit format(std::uint16_t value) noexcept : value_(value) {}
  constexpr format(ice::color color) noexcept : format(static_cast<std::uint16_t>(color)) {}
  constexpr format(ice::style style) noexcept : format(static_cast<std::uint16_t>(style)) {}

  constexpr explicit operator bool() const noexcept
  {
    return value_ != 0;
  }

  constexpr format& operator|(ice::color color) noexcept
  {
    value_ = static_cast<std::uint16_t>((value_ & 0xF0) | static_cast<std::uint16_t>(color));
    return *this;
  }

  constexpr format& operator|(ice::style style) noexcept
  {
    value_ = static_cast<std::uint16_t>(value_ | static_cast<std::uint16_t>(style));
    return *this;
  }

  constexpr ice::color color() const noexcept
  {
    return static_cast<ice::color>(value_ & 0x0F);
  }

  constexpr bool is(ice::style style) const noexcept
  {
    return value_ & static_cast<std::uint16_t>(style);
  }

private:
  std::uint16_t value_ = 0;
};

constexpr inline format operator|(color lhs, style rhs) noexcept
{
  return format{ static_cast<std::uint16_t>(static_cast<std::uint16_t>(lhs) | static_cast<std::uint16_t>(rhs)) };
}

constexpr inline format operator|(style lhs, color rhs) noexcept
{
  return format{ static_cast<std::uint16_t>(static_cast<std::uint16_t>(lhs) | static_cast<std::uint16_t>(rhs)) };
}

constexpr inline format operator|(style lhs, style rhs) noexcept
{
  return format{ static_cast<std::uint16_t>(static_cast<std::uint16_t>(lhs) | static_cast<std::uint16_t>(rhs)) };
}

inline void print(FILE* stream, char character) noexcept
{
  std::fputc(character, stream);
}

void print(FILE* stream, format format, char character) noexcept;

inline void print(FILE* stream, const char* buffer) noexcept
{
  std::fputs(buffer, stream);
}

void print(FILE* stream, format format, const char* buffer) noexcept;

template <typename Arg, typename... Args>
inline void print(FILE* stream, fmt::string_view message, Arg&& arg, Args&&... args)
{
  print(stream, fmt::format(message, std::forward<Arg>(arg), std::forward<Args>(args)...).data());
}

template <typename Arg, typename... Args>
inline void print(FILE* stream, format format, fmt::string_view message, Arg&& arg, Args&&... args)
{
  print(stream, format, fmt::format(message, std::forward<Arg>(arg), std::forward<Args>(args)...).data());
}

void printf(FILE* stream, const char* buffer, ...) noexcept;
void printf(FILE* stream, format format, const char* buffer, ...) noexcept;

inline void print(char character) noexcept
{
  print(stdout, character);
}

inline void print(format format, char character) noexcept
{
  print(stdout, format, character);
}

inline void print(const char* buffer) noexcept
{
  print(stdout, buffer);
}

inline void print(format format, const char* buffer) noexcept
{
  print(stdout, format, buffer);
}

template <typename Arg, typename... Args>
inline void print(fmt::string_view message, Arg&& arg, Args&&... args)
{
  print(stdout, message, std::forward<Arg>(arg), std::forward<Args>(args)...);
}

template <typename Arg, typename... Args>
inline void print(format format, fmt::string_view message, Arg&& arg, Args&&... args)
{
  print(stdout, format, message, std::forward<Arg>(arg), std::forward<Args>(args)...);
}

void printf(const char* buffer, ...) noexcept;
void printf(format format, const char* buffer, ...) noexcept;

}  // namespace ice
