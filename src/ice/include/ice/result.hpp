#pragma once
#include <ice/config.hpp>
#include <system_error>
#include <type_traits>
#include <cassert>

namespace ice {

template <typename T>
class result {
public:
  static_assert(!std::is_reference_v<T>, "T must not be a reference");
  static_assert(!std::is_same_v<T, std::remove_cv<std::error_code>>, "T must not be std::error_code");

  using value_type = T;
  using error_type = std::error_code;

  result(std::enable_if_t<std::is_default_constructible_v<T>>) noexcept : error_(false)
  {
    new (&storage_.value) value_type{};
  }

  template <typename... Args>
  result(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) : error_(false)
  {
    new (&storage_.value) value_type{ std::forward<Args>(args)... };
  }

  result(std::error_code ec) noexcept : error_(true)
  {
    new (&storage_.error) error_type{ ec };
  }

#if 1
  // clang-format off
  result(std::enable_if_t<std::is_move_constructible_v<T>, result&&> other) noexcept(std::is_nothrow_move_constructible_v<T>) : error_(other.error_)
  {
    if (error_) {
      new (&storage_.error) error_type{ other.storage_.error };
    } else {
      new (&storage_.value) value_type{ std::move(other.storage_.value) };
    }
  }

  result(std::enable_if_t<std::is_copy_constructible_v<T>, const result&> other) noexcept(std::is_nothrow_copy_constructible_v<T>) : error_(other.error_)
  {
    if (error_) {
      new (&storage_.error) error_type{ other.storage_.error };
    } else {
      new (&storage_.value) value_type{ other.storage_.value };
    }
  }

  result& operatpr=(std::enable_if_t<std::is_move_assignable_v<T>, result&&> other) noexcept(std::is_nothrow_move_assignable_v<T>)
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
      new (&storage_.value) value_type{ std::move(other.storage_.value) };
    }
  }

  result& operatpr=(std::enable_if_t<std::is_copy_assignable_v<T>, const result&> other) noexcept(std::is_nothrow_copy_assignable_v<T>)
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
  }
  // clang-format on
#endif

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

  constexpr T&& operator*() && noexcept(std::is_nothrow_move_assignable_v<T>&& std::is_nothrow_move_constructible_v<T>)
  {
    assert(!error_);
    return std::move(storage_.value);
  }

  constexpr T& value() & noexcept(ICE_NO_EXCEPTIONS)
  {
#if ICE_EXCEPTIONS
    if (error_) {
      throw std::system_error(storage_.error, "bad result");
    }
#else
    assert(!error_);
#endif
    return storage_.value;
  }

  constexpr const T& value() const& noexcept(ICE_NO_EXCEPTIONS)
  {
#if ICE_EXCEPTIONS
    if (error_) {
      throw std::system_error(storage_.error, "bad result");
    }
#else
    assert(!error_);
#endif
    return storage_.value;
  }

  constexpr T&& value() && noexcept(ICE_NO_EXCEPTIONS)
  {
#if ICE_EXCEPTIONS
    if (error_) {
      throw std::system_error(storage_.error, "bad result");
    }
#else
    assert(!error_);
#endif
    return std::move(storage_.value);
  }

  constexpr std::error_code error() const noexcept
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

}  // namespace ice
