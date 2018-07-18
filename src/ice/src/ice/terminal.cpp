#include "ice/terminal.hpp"

#if ICE_OS_WIN32
#  include <io.h>
#  include <windows.h>
#else
#  include <unistd.h>
#endif

namespace ice::terminal {
namespace {

class terminal_manager {
public:
  terminal_manager() noexcept :
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
    if (!format || !is_tty(stream)) {
      return;
    }
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
    case color::grey:    std::fputs("\033[38;2;130;130;130m", stream); break;  // mintty cannot display "\033[30m"
    case color::red:     std::fputs("\033[31m", stream); break;
    case color::green:   std::fputs("\033[32m", stream); break;
    case color::yellow:  std::fputs("\033[33m", stream); break;
    case color::blue:    std::fputs("\033[34m", stream); break;
    case color::magenta: std::fputs("\033[35m", stream); break;
    case color::cyan:    std::fputs("\033[36m", stream); break;
    case color::white:   std::fputs("\033[37m", stream); break;
    }
    // clang-format on
    if (format.is(style::bold)) {
      std::fputs("\033[1m", stream);
    }
    if (format.is(style::dark)) {
      std::fputs("\033[2m", stream);
    }
    if (format.is(style::underline)) {
      std::fputs("\033[4m", stream);
    }
    if (format.is(style::blink)) {
      std::fputs("\033[5m", stream);
    }
    if (format.is(style::reverse)) {
      std::fputs("\033[7m", stream);
    }
    if (format.is(style::conceal)) {
      std::fputs("\033[8m", stream);
    }
#endif
  }

  void reset(FILE* stream) noexcept
  {
    if (!is_tty(stream)) {
      return;
    }
#if ICE_OS_WIN32
    ::SetConsoleTextAttribute(handle(stream), attributes(stream));
#else
    std::fputs("\033[00m", stream);
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
    if (!format.is(style::dark)) {
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

terminal_manager g_terminal_manager;

}  // namespace

bool is_tty(FILE* stream) noexcept
{
  return g_terminal_manager.is_tty(stream);
}

void set(FILE* stream, format format) noexcept
{
  g_terminal_manager.set(stream, format);
}

void reset(FILE* stream) noexcept
{
  g_terminal_manager.reset(stream);
}

}  // namespace ice::terminal
