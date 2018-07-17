#pragma once
#include <ice/config.hpp>
#include <system_error>
#include <type_traits>

namespace ice {

enum class errc {
  eof = 1,
  invalid_address,
  version_mismatch,
};

const std::error_category& native_category();
const std::error_category& system_category();
const std::error_category& domain_category();

template <typename T>
inline std::error_code make_error_code(T ev) noexcept
{
  if constexpr (std::is_same_v<T, std::errc>) {
    return { static_cast<int>(ev), ice::system_category() };
  } else if constexpr (std::is_same_v<T, ice::errc>) {
    return { static_cast<int>(ev), ice::domain_category() };
  } else if constexpr (std::is_same_v<T, std::error_code>) {
    return ev;
  } else {
    return { static_cast<int>(ev), ice::native_category() };
  }
}

template <typename T>
inline std::error_code make_error_code(T ev, const std::error_category& category) noexcept
{
  return { static_cast<int>(ev), category };
}

// clang-format off

#if 0
#include <ice/terminal.hpp>
#include <fmt/format.h>
#include <stdexcept>
#include <cstdlib>

class runtime_error : public std::runtime_error {
public:
  explicit runtime_error(const std::string& what) :
    std::runtime_error(what)
  {}

  explicit runtime_error(const char* what) :
    std::runtime_error(what)
  {}
};

class system_error : public std::system_error {
public:
  explicit system_error(std::error_code ec) :
    std::system_error(ec)
  {}

  explicit system_error(std::error_code ec, const std::string& what) :
    std::system_error(ec, what)
  {}

  explicit system_error(std::error_code ec, const char* what) :
    std::system_error(ec, what)
  {}

  template <typename T>
  explicit system_error(T ev) :
    std::system_error(ice::make_error_code(ev))
  {}

  template <typename T>
  explicit system_error(T ev, const std::string& what) :
    std::system_error(ice::make_error_code(ev), what)
  {}

  template <typename T>
  explicit system_error(T ev, const char* what) :
    std::system_error(ice::make_error_code(ev), what)
  {}

  template <typename T>
  explicit system_error(T ev, const std::error_category& category) :
    std::system_error(ice::make_error_code(ev, category))
  {}

  template <typename T>
  explicit system_error(T ev, const std::error_category& category, const std::string& what) :
    std::system_error(ice::make_error_code(ev, category), what)
  {}

  template <typename T>
  explicit system_error(T ev, const std::error_category& category, const char* what) :
    std::system_error(ice::make_error_code(ev, category), what)
  {}
};

// clang-format on

template <typename T>
inline void throw_exception(T e) noexcept(ICE_NO_EXCEPTIONS)
{
#  if ICE_NO_EXCEPTIONS
  terminal::manager manager{ stderr, color::red | style::bold };
  if constexpr (std::is_base_of_v<std::system_error, T>) {
    const auto ec = e.code();
    fmt::print(stderr, "{} error {}: {} ({})\n", ec.category().name(), ec.value(), e.what(), ec.message());
  } else {
    fmt::print(stderr, "critical error: {}\n", e.what());
  }
  manager.reset();
  std::abort();
#  else
  throw e;
#  endif
}

template <typename T>
inline void throw_error(T ev) noexcept(ICE_NO_EXCEPTIONS)
{
  throw_exception(ice::system_error{ ev });
}

template <typename T>
inline void throw_error(T ev, const std::string& what) noexcept(ICE_NO_EXCEPTIONS)
{
  throw_exception(ice::system_error{ ev, what });
}

template <typename T>
inline void throw_error(T ev, const char* what) noexcept(ICE_NO_EXCEPTIONS)
{
  throw_exception(ice::system_error{ ev, what });
}

template <typename T>
inline void throw_error(T ev, const std::error_category& category) noexcept(ICE_NO_EXCEPTIONS)
{
  throw_exception(ice::system_error{ ev, category });
}

template <typename T>
inline void throw_error(T ev, const std::error_category& category, const std::string& what) noexcept(ICE_NO_EXCEPTIONS)
{
  throw_exception(ice::system_error{ ev, category, what });
}

template <typename T>
inline void throw_error(T ev, const std::error_category& category, const char* what) noexcept(ICE_NO_EXCEPTIONS)
{
  throw_exception(ice::system_error{ ev, category, what });
}

template <typename T>
inline void throw_on_error(T ev) noexcept(ICE_NO_EXCEPTIONS)
{
  if (const auto ec = make_error_code(ev)) {
    throw_error(ec);
  }
}

template <typename T>
inline void throw_on_error(T ev, const std::string& what) noexcept(ICE_NO_EXCEPTIONS)
{
  if (const auto ec = make_error_code(ev)) {
    throw_error(ec, what);
  }
}

template <typename T>
inline void throw_on_error(T ev, const char* what) noexcept(ICE_NO_EXCEPTIONS)
{
  if (const auto ec = make_error_code(ev)) {
    throw_error(ec, what);
  }
}

template <typename T>
inline void throw_on_error(T ev, const std::error_category& category) noexcept(ICE_NO_EXCEPTIONS)
{
  if (const auto ec = make_error_code(ev, category)) {
    throw_error(ec);
  }
}

template <typename T>
inline void throw_on_error(T ev, const std::error_category& category, const std::string& what) noexcept(ICE_NO_EXCEPTIONS)
{
  if (const auto ec = make_error_code(ev, category)) {
    throw_error(ec, what);
  }
}

template <typename T>
inline void throw_on_error(T ev, const std::error_category& category, const char* what) noexcept(ICE_NO_EXCEPTIONS)
{
  if (const auto ec = make_error_code(ev, category)) {
    throw_error(ec, what);
  }
}
#endif

}  // namespace ice
