#pragma once
#include <ice/config.hpp>
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
  constexpr format(color color) noexcept : format(static_cast<std::uint16_t>(color)) {}
  constexpr format(style style) noexcept : format(static_cast<std::uint16_t>(style)) {}

  constexpr explicit operator bool() const noexcept
  {
    return value_ != 0;
  }

  constexpr format& operator|(color color) noexcept
  {
    value_ = static_cast<std::uint16_t>((value_ & 0xF0) | static_cast<std::uint16_t>(color));
    return *this;
  }

  constexpr format& operator|(style style) noexcept
  {
    value_ = static_cast<std::uint16_t>(value_ | static_cast<std::uint16_t>(style));
    return *this;
  }

  constexpr color color() const noexcept
  {
    return static_cast<ice::color>(value_ & 0x0F);
  }

  constexpr bool is(style style) const noexcept
  {
    return value_ & static_cast<std::uint16_t>(style);
  }

private:
  std::uint16_t value_ = 0;
};

inline constexpr format operator|(color lhs, style rhs) noexcept
{
  return format{ static_cast<std::uint16_t>(static_cast<std::uint16_t>(lhs) | static_cast<std::uint16_t>(rhs)) };
}

inline constexpr format operator|(style lhs, color rhs) noexcept
{
  return format{ static_cast<std::uint16_t>(static_cast<std::uint16_t>(lhs) | static_cast<std::uint16_t>(rhs)) };
}

inline constexpr format operator|(style lhs, style rhs) noexcept
{
  return format{ static_cast<std::uint16_t>(static_cast<std::uint16_t>(lhs) | static_cast<std::uint16_t>(rhs)) };
}

namespace terminal {

bool is_tty(FILE* stream) noexcept;
void set(FILE* stream, format format) noexcept;
void reset(FILE* stream) noexcept;

class manager {
public:
  manager() noexcept = default;
  manager(FILE* stream, format format) noexcept : stream_(stream)
  {
    set(format);
  }

  manager(manager&& other) = delete;
  manager(const manager& other) = delete;
  manager& operator=(manager&& other) = delete;
  manager& operator=(const manager& other) = delete;

  ~manager()
  {
    reset();
  }

  void set(format format) noexcept
  {
    reset();
    if (format) {
      terminal::set(stream_, format);
      reset_ = true;
    }
  }

  void reset() noexcept
  {
    if (reset_) {
      terminal::reset(stream_);
      reset_ = false;
    }
  }

private:
  FILE* stream_ = nullptr;
  bool reset_ = false;
};

}  // namespace terminal
}  // namespace ice
