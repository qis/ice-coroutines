#include "print.hpp"
#include <ice/error.hpp>
#include <cstdarg>

#if ICE_OS_WIN32
#  include <io.h>
#  include <windows.h>
#else
#  include <unistd.h>
#endif

namespace ice {
namespace {

class terminal {
public:
  terminal() noexcept :
#if ICE_OS_WIN32
    stdout_is_tty_(_isatty(_fileno(stdout)) ? true : false), stderr_is_tty_(_isatty(_fileno(stderr)) ? true : false),
    stdout_handle_(::GetStdHandle(STD_OUTPUT_HANDLE)), stderr_handle_(::GetStdHandle(STD_ERROR_HANDLE))
#else
    stdout_is_tty_(isatty(fileno(stdout)) ? true : false), stderr_is_tty_(isatty(fileno(stderr)) ? true : false)
#endif
  {
#if ICE_OS_WIN32
    CONSOLE_SCREEN_BUFFER_INFO stdout_info = {};
    if (::GetConsoleScreenBufferInfo(stdout_handle_, &stdout_info)) {
      stdout_default_attributes_ = stdout_info.wAttributes;
    }
    CONSOLE_SCREEN_BUFFER_INFO stderr_info = {};
    if (::GetConsoleScreenBufferInfo(stderr_handle_, &stderr_info)) {
      stderr_default_attributes_ = stderr_info.wAttributes;
    }
#endif
  }

  bool is_tty(FILE* stream) noexcept
  {
    return stream == stdout ? stdout_is_tty_ : stream == stderr ? stderr_is_tty_ : false;
  }

  void set(FILE* stream, format format) noexcept
  {
    assert(format);
    assert(is_tty(stream));
#if ICE_OS_WIN32
    // clang-format off
    switch (format.color()) {
    case color::none:    break;
    case color::grey:    set(stream, format, 0); break;
    case color::red:     set(stream, format, FOREGROUND_RED); break;
    case color::green:   set(stream, format, FOREGROUND_GREEN); break;
    case color::yellow:  set(stream, format, FOREGROUND_GREEN | FOREGROUND_RED); break;
    case color::blue:    set(stream, format, FOREGROUND_BLUE); break;
    case color::magenta: set(stream, format, FOREGROUND_BLUE | FOREGROUND_RED); break;
    case color::cyan:    set(stream, format, FOREGROUND_BLUE | FOREGROUND_GREEN); break;
    case color::white:   set(stream, format, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED); break;
    }
    // clang-format on
#else
    // clang-format off
    switch (format.color()) {
    case color::none:    break;
    case color::grey:    print(stream, "\033[38;2;130;130;130m"); break;  // // mintty cannot display "\033[30m"
    case color::red:     print(stream, "\033[31m"); break;
    case color::green:   print(stream, "\033[32m"); break;
    case color::yellow:  print(stream, "\033[33m"); break;
    case color::blue:    print(stream, "\033[34m"); break;
    case color::magenta: print(stream, "\033[35m"); break;
    case color::cyan:    print(stream, "\033[36m"); break;
    case color::white:   print(stream, "\033[37m"); break;
    }
    // clang-format on
    if (format.is(style::bold)) {
      print(stream, "\033[1m");
    }
    if (format.is(style::dark)) {
      print(stream, "\033[2m");
    }
    if (format.is(style::underline)) {
      print(stream, "\033[4m");
    }
    if (format.is(style::blink)) {
      print(stream, "\033[5m");
    }
    if (format.is(style::reverse)) {
      print(stream, "\033[7m");
    }
    if (format.is(style::conceal)) {
      print(stream, "\033[8m");
    }
#endif
  }

  void reset(FILE* stream) noexcept
  {
    assert(is_tty(stream));
#if ICE_OS_WIN32
    ::SetConsoleTextAttribute(handle(stream), attributes(stream));
#else
    print(stream, "\033[00m");
#endif
  }

private:
#if ICE_OS_WIN32
  HANDLE handle(FILE* stream) noexcept
  {
    return stream == stdout ? stdout_handle_ : stream == stderr ? stderr_handle_ : nullptr;
  }

  WORD attributes(FILE* stream) noexcept
  {
    return stream == stdout ? stdout_default_attributes_ : stream == stderr ? stderr_default_attributes_ : 0;
  }

  void set(FILE* stream, format format, WORD color) noexcept
  {
    auto base = attributes(stream);
    base &= ~(base & 0x0F);
    if (!format.is(ice::style::dark)) {
      base |= FOREGROUND_INTENSITY;
    }
    ::SetConsoleTextAttribute(handle(stream), base | color);
  }
#endif

  bool stdout_is_tty_ = false;
  bool stderr_is_tty_ = false;

#if ICE_OS_WIN32
  HANDLE stdout_handle_ = nullptr;
  HANDLE stderr_handle_ = nullptr;
  WORD stdout_default_attributes_ = 0;
  WORD stderr_default_attributes_ = 0;
#endif
};

terminal g_terminal;

}  // namespace

void print(FILE* stream, format format, char character) noexcept
{
  if (format && g_terminal.is_tty(stream)) {
    g_terminal.set(stream, format);
    print(stream, character);
    g_terminal.reset(stream);
  } else {
    print(stream, character);
  }
}

void print(FILE* stream, format format, const char* buffer) noexcept
{
  if (format && g_terminal.is_tty(stream)) {
    g_terminal.set(stream, format);
    print(stream, buffer);
    g_terminal.reset(stream);
  } else {
    print(stream, buffer);
  }
}

void printf(FILE* stream, const char* buffer, ...) noexcept
{
  va_list args;
  va_start(args, buffer);
  std::vfprintf(stream, buffer, args);
  va_end(args);
}

void printf(FILE* stream, format format, const char* buffer, ...) noexcept
{
  va_list args;
  va_start(args, buffer);
  if (format && g_terminal.is_tty(stream)) {
    g_terminal.set(stream, format);
    std::vfprintf(stream, buffer, args);
    g_terminal.reset(stream);
  } else {
    std::vfprintf(stream, buffer, args);
  }
  va_end(args);
}

void printf(const char* buffer, ...) noexcept
{
  va_list args;
  va_start(args, buffer);
  std::vfprintf(stdout, buffer, args);
  va_end(args);
}

void printf(format format, const char* buffer, ...) noexcept
{
  va_list args;
  va_start(args, buffer);
  if (format && g_terminal.is_tty(stdout)) {
    g_terminal.set(stdout, format);
    std::vfprintf(stdout, buffer, args);
    g_terminal.reset(stdout);
  } else {
    std::vfprintf(stdout, buffer, args);
  }
  va_end(args);
}

}  // namespace ice
