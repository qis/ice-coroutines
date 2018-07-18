#pragma once
#include <ice/config.hpp>
#include <ice/terminal.hpp>
#include <fmt/format.h>
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
    return { static_cast<int>(ev), system_category() };
  } else if constexpr (std::is_same_v<T, errc>) {
    return { static_cast<int>(ev), domain_category() };
  } else if constexpr (std::is_same_v<T, std::error_code>) {
    return ev;
  } else {
    return { static_cast<int>(ev), native_category() };
  }
}

template <typename T>
inline std::error_code make_error_code(T ev, const std::error_category& category) noexcept
{
  return { static_cast<int>(ev), category };
}

template <typename T>
inline void throw_exception(T e) noexcept(ICE_NO_EXCEPTIONS)
{
#if ICE_NO_EXCEPTIONS
  terminal::manager manager{ stderr, color::red | style::bold };
  if constexpr (std::is_base_of_v<std::system_error, T>) {
    const auto ec = e.code();
    fmt::print(stderr, "{} error {}: {} ({})\n", ec.category().name(), ec.value(), e.what(), ec.message());
  } else {
    fmt::print(stderr, "critical error: {}\n", e.what());
  }
  manager.reset();
  std::abort();
#else
  throw e;
#endif
}

}  // namespace ice
