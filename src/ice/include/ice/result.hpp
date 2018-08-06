#pragma once
#include <ice/config.hpp>
#include <ice/utility.hpp>
#include <system_error>
#include <type_traits>
#include <cassert>

namespace ice {
namespace detail {

template <typename T>
struct result_storage {
  using value_type = T;
  using error_type = std::error_code;

  template <typename... Args>
  result_storage(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args&&...>) :
    value(std::forward<Args>(args)...), has_value(true)
  {}

  explicit result_storage(error_type error) noexcept : error(error), has_value(false) {}

  result_storage(result_storage&& other) noexcept(std::is_nothrow_move_constructible_v<T>) : has_value(other.has_value)
  {
    if (has_value) {
      new (static_cast<void*>(&value)) value_type(std::move(other.value));
    } else {
      new (static_cast<void*>(&error)) error_type(std::move(other.error));
    }
  }

  result_storage(const result_storage& other) noexcept(std::is_nothrow_copy_constructible_v<T>) :
    has_value(other.has_value)
  {
    if (has_value) {
      new (static_cast<void*>(&value)) value_type(other.value);
    } else {
      new (static_cast<void*>(&error)) error_type(other.error);
    }
  }

  result_storage& operator=(result_storage&& other) noexcept(std::is_nothrow_move_assignable_v<T>)
  {
    if (has_value) {
      value.~value_type();
    } else {
      error.~error_type();
    }
    has_value = other.has_value;
    if (has_value) {
      new (static_cast<void*>(&value)) value_type(std::move(other.value));
    } else {
      new (static_cast<void*>(&error)) error_type(std::move(other.error));
    }
    return *this;
  }

  result_storage& operator=(const result_storage& other) noexcept(std::is_nothrow_copy_assignable_v<T>)
  {
    if (has_value) {
      value.~value_type();
    } else {
      error.~error_type();
    }
    has_value = other.has_value;
    if (has_value) {
      new (static_cast<void*>(&value)) value_type(other.value);
    } else {
      new (static_cast<void*>(&error)) error_type(other.error);
    }
    return *this;
  }

  ~result_storage() noexcept(std::is_nothrow_destructible_v<T>)
  {
    if (has_value) {
      value.~value_type();
    } else {
      error.~error_type();
    }
  }

  union {
    value_type value;
    error_type error;
  };
  bool has_value;
};

}  // namespace detail

template <typename T>
class result : private delete_move_constructor<T>,
               private delete_copy_constructor<T>,
               private delete_move_assignment<T>,
               private delete_copy_assignment<T> {
public:
  using value_type = T;
  using error_type = std::error_code;

  template <typename... Args, std::enable_if_t<std::is_constructible_v<value_type, Args&&...>>* = nullptr>
  result(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args&&...>) : storage_(std::forward<Args>(args)...)
  {}

  explicit result(error_type error) noexcept : storage_(error) {}

  result(result&& other) = default;
  result(const result& other) = default;
  result& operator=(result&& other) = default;
  result& operator=(const result& other) = default;

  constexpr bool has_value() const noexcept
  {
    return storage_.has_value;
  }

  constexpr explicit operator bool() const noexcept
  {
    return storage_.has_value;
  }

  constexpr value_type* operator->()
  {
    assert(has_value());
    return &storage_.value;
  }

  constexpr const value_type* operator->() const
  {
    assert(has_value());
    return &storage_.value;
  }

  constexpr value_type& operator*() &
  {
    assert(has_value());
    return storage_.value;
  }

  constexpr value_type&& operator*() &&
  {
    assert(has_value());
    return std::move(storage_.value);
  }

  constexpr const value_type& operator*() const&
  {
    assert(has_value());
    return storage_.value;
  }

  constexpr const value_type&& operator*() const&&
  {
    assert(has_value());
    return std::move(storage_.value);
  }

  constexpr value_type& value() &
  {
#if ICE_EXCEPTIONS
    if (!has_value()) {
      throw std::system_error(storage_.error, "bad result");
    }
#else
    assert(has_value());
#endif
    return storage_.value;
  }

  constexpr value_type&& value() &&
  {
#if ICE_EXCEPTIONS
    if (!has_value()) {
      throw std::system_error(storage_.error, "bad result");
    }
#else
    assert(has_value());
#endif
    return std::move(storage_.value);
  }

  constexpr const value_type& value() const&
  {
#if ICE_EXCEPTIONS
    if (!has_value()) {
      throw std::system_error(storage_.error, "bad result");
    }
#else
    assert(has_value());
#endif
    return storage_.value;
  }

  constexpr const value_type&& value() const&&
  {
#if ICE_EXCEPTIONS
    if (!has_value()) {
      throw std::system_error(storage_.error, "bad result");
    }
#else
    assert(has_value());
#endif
    return std::move(storage_.value);
  }

  error_type error() const noexcept
  {
    return has_value() ? std::error_code{} : storage_.error;
  }

private:
  detail::result_storage<T> storage_;
};

template <>
class result<void> {
public:
  using value_type = void;
  using error_type = std::error_code;

  result() noexcept = default;
  explicit result(error_type error) noexcept : error_(error) {}

  result(result&& other) = default;
  result(const result& other) = default;
  result& operator=(result&& other) = default;
  result& operator=(const result& other) = default;

  constexpr bool has_value() const noexcept
  {
    return has_value_;
  }

  constexpr explicit operator bool() const noexcept
  {
    return has_value_;
  }

  constexpr value_type operator*() const
  {
    assert(has_value_);
  }

  value_type value() const
  {
#if ICE_EXCEPTIONS
    if (!has_value_) {
      throw std::system_error(error_, "bad result");
    }
#else
    assert(has_value_);
#endif
  }

  error_type error() const noexcept
  {
    return error_;
  }

private:
  bool has_value_ = true;
  error_type error_;
};

template <typename T, typename U>
constexpr bool operator==(const result<T>& lhs, const result<U>& rhs)
{
  return (lhs.has_value() != rhs.has_value()) ? false : (!lhs.has_value() ? lhs.error() == rhs.error() : *lhs == *rhs);
}

template <typename T, typename U>
constexpr bool operator!=(const result<T>& lhs, const result<U>& rhs)
{
  return (lhs.has_value() != rhs.has_value()) ? true : (!lhs.has_value() ? lhs.error() != rhs.error() : *lhs != *rhs);
}

template <typename T, typename U>
constexpr bool operator==(const result<T>& x, const U& v)
{
  return x.has_value() ? *x == v : false;
}

template <typename T, typename U>
constexpr bool operator==(const U& v, const result<T>& x)
{
  return x.has_value() ? *x == v : false;
}

template <typename T, typename U>
constexpr bool operator!=(const result<T>& x, const U& v)
{
  return x.has_value() ? *x != v : true;
}

template <typename T, typename U>
constexpr bool operator!=(const U& v, const result<T>& x)
{
  return x.has_value() ? *x != v : true;
}

template <typename T>
constexpr bool operator==(const result<T>& x, const std::error_code& e)
{
  return x.has_value() ? false : x.error() == e;
}

template <typename T>
constexpr bool operator==(const std::error_code& e, const result<T>& x)
{
  return x.has_value() ? false : x.error() == e;
}

}  // namespace ice
