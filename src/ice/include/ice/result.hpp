#pragma once
#include <ice/config.hpp>
#include <system_error>
#include <type_traits>
#include <cassert>

namespace ice {

template <typename T, typename... Args>
struct is_error {
  constexpr static bool value = sizeof...(Args) == 0 && std::is_same_v<std::decay_t<T>, std::error_code>;
};

template <typename T, typename... Args>
inline constexpr bool is_error_v = is_error<T, Args...>::value;

#if ICE_HAS_CONSTRAINTS

template <typename T>
class result {
public:
  static_assert(!std::is_reference_v<T>, "T must not be a reference");
  static_assert(!std::is_same_v<T, std::remove_cv<std::error_code>>, "T must not be std::error_code");

  using value_type = T;
  using error_type = std::error_code;

  // clang-format off

  result() noexcept(std::is_nothrow_default_constructible_v<T>) requires(std::is_default_constructible_v<T>) : error_(false)
  {
    new (&storage_.value) value_type{};
  }

  template <typename... Args>
  result(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) : error_(is_error_v<Args...>)
  {
    if constexpr (is_error_v<Args...>) {
      new (&storage_.error) error_type{ std::forward<Args>(args)... };
    } else {
      new (&storage_.value) value_type{ std::forward<Args>(args)... };
    }
  }

  result(result&& other) noexcept(std::is_nothrow_move_constructible_v<T>) requires(std::is_move_constructible_v<T>) : error_(other.error_)
  {
    if (error_) {
      new (&storage_.error) error_type{ std::move(other.storage_.error) };
    } else {
      new (&storage_.value) value_type{ std::move(other.storage_.value) };
    }
  }

  result& operator=(result&& other) noexcept(std::is_nothrow_move_assignable_v<T>) requires(std::is_move_assignable_v<T>)
  {
    if (error_) {
      storage_.error.~error_type();
    } else {
      storage_.value.~value_type();
    }
    error_ = other.error_;
    if (error_) {
      new (&storage_.error) error_type{ std::move(other.storage_.error) };
    } else {
      new (&storage_.value) value_type{ std::move(other.storage_.value) };
    }
    return *this;
  }

  result(const result& other) noexcept(std::is_nothrow_copy_constructible_v<T>) requires(std::is_copy_constructible_v<T>) : error_(other.error_)
  {
    if (error_) {
      new (&storage_.error) error_type{ other.storage_.error };
    } else {
      new (&storage_.value) value_type{ other.storage_.value };
    }
  }

  result& operator=(const result& other) noexcept(std::is_nothrow_copy_constructible_v<T>) requires(std::is_copy_constructible_v<T>)
  {
    if (error_) {
      storage_.error.~error_type();
    } else {
      storage_.value.~value_type();
    }
    error_ = other.error_;
    if (error_) {
      new (&storage_.error) error_type{ other.storage_.error };
    } else {
      new (&storage_.value) value_type{ other.storage_.value };
    }
    return *this;
  }

  // clang-format on

  ~result() noexcept(std::is_nothrow_destructible_v<T>)
  {
    if (error_) {
      storage_.error.~error_type();
    } else {
      storage_.value.~value_type();
    }
  }

  constexpr explicit operator bool() const noexcept
  {
    return !error_;
  }

  constexpr T& operator*() & noexcept
  {
    assert(!error_);
    return storage_.value;
  }

  constexpr const T& operator*() const& noexcept
  {
    assert(!error_);
    return storage_.value;
  }

  constexpr T&& operator*() && noexcept
  {
    assert(!error_);
    return std::move(storage_.value);
  }

  constexpr T& value() & noexcept(ICE_NO_EXCEPTIONS)
  {
#  if ICE_EXCEPTIONS
    if (error_) {
      throw std::system_error(storage_.error, "bad result");
    }
#  else
    assert(!error_);
#  endif
    return storage_.value;
  }

  constexpr const T& value() const& noexcept(ICE_NO_EXCEPTIONS)
  {
#  if ICE_EXCEPTIONS
    if (error_) {
      throw std::system_error(storage_.error, "bad result");
    }
#  else
    assert(!error_);
#  endif
    return storage_.value;
  }

  constexpr T&& value() && noexcept(ICE_NO_EXCEPTIONS)
  {
#  if ICE_EXCEPTIONS
    if (error_) {
      throw std::system_error(storage_.error, "bad result");
    }
#  else
    assert(!error_);
#  endif
    return std::move(storage_.value);
  }

  std::error_code error() const noexcept
  {
    if (error_) {
      return storage_.error;
    }
    return {};
  }

private:
  union storage {
    value_type value;
    error_type error;
    storage() {}
    ~storage() {}
  } storage_;
  bool error_;
};

#else

namespace detail {

template <typename T>
struct result_storage {
  using value_type = T;
  using error_type = std::error_code;

  result_storage() noexcept(std::is_nothrow_default_constructible_v<T>) : error_(false)
  {
    new (&storage_.value) value_type{};
  }

  template <typename... Args>
  result_storage(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) : error_(is_error_v<Args...>)
  {
    if constexpr (is_error_v<Args...>)
    {
      new (&storage_.error) error_type{ std::forward<Args>(args)... };
    } else {
      new (&storage_.value) value_type{ std::forward<Args>(args)... };
    }
  }

  result_storage(result_storage&& other) noexcept(std::is_nothrow_move_constructible_v<T>) : error_(other.error_)
  {
    if (error_) {
      new (&storage_.error) error_type{ std::move(other.storage_.error) };
    } else {
      new (&storage_.value) value_type{ std::move(other.storage_.value) };
    }
  }

  result_storage& operator=(result_storage&& other) noexcept(std::is_nothrow_move_assignable_v<T>)
  {
    if (error_) {
      storage_.error.~error_type();
    } else {
      storage_.value.~value_type();
    }
    error_ = other.error_;
    if (error_) {
      new (&storage_.error) error_type{ std::move(other.storage_.error) };
    } else {
      new (&storage_.value) value_type{ std::move(other.storage_.value) };
    }
    return *this;
  }

  result_storage(const result_storage& other) noexcept(std::is_nothrow_copy_constructible_v<T>) : error_(other.error_)
  {
    if (error_) {
      new (&storage_.error) error_type{ other.storage_.error };
    } else {
      new (&storage_.value) value_type{ other.storage_.value };
    }
  }

  result_storage& operator=(const result_storage& other) noexcept(std::is_nothrow_copy_constructible_v<T>)
  {
    if (error_) {
      storage_.error.~error_type();
    } else {
      storage_.value.~value_type();
    }
    error_ = other.error_;
    if (error_) {
      new (&storage_.error) error_type{ other.storage_.error };
    } else {
      new (&storage_.value) value_type{ other.storage_.value };
    }
    return *this;
  }

  ~result_storage() noexcept(std::is_nothrow_destructible_v<T>)
  {
    if (error_) {
      storage_.error.~error_type();
    } else {
      storage_.value.~value_type();
    }
  }

  union storage {
    value_type value;
    error_type error;
    storage() {}
    ~storage() {}
  } storage_;
  bool error_;
};

}  // namespace detail

#endif

}  // namespace ice
