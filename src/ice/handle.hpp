#pragma once
#include <ice/config.hpp>
#include <type_traits>
#include <utility>

namespace ice {

template <typename T>
struct is_handle {
  constexpr static auto value = std::is_integral_v<T> || std::is_pointer_v<T>;
};

template <typename T>
constexpr inline auto is_handle_v = ice::is_handle<T>::value;

template <typename T, auto I>
class handle_view {
public:
  using value_type = std::decay_t<T>;

  constexpr static value_type invalid_value() noexcept
  {
    if constexpr (std::is_pointer_v<value_type>) {
      if constexpr (std::is_null_pointer_v<decltype(I)>) {
        return I;
      } else {
        return reinterpret_cast<value_type>(I);
      }
    } else {
      if constexpr (std::is_pointer_v<decltype(I)>) {
        return reinterpret_cast<value_type>(I);
      } else {
        return static_cast<value_type>(I);
      }
    }
  }

  constexpr handle_view() noexcept = default;

  template <typename V, typename = std::enable_if_t<ice::is_handle_v<V>>>
  constexpr explicit handle_view(V value) noexcept
  {
    if constexpr (std::is_pointer_v<value_type> || std::is_pointer_v<V>) {
      value_ = reinterpret_cast<value_type>(value);
    } else {
      value_ = static_cast<value_type>(value);
    }
  }

  constexpr handle_view(handle_view&& other) noexcept = default;
  constexpr handle_view& operator=(handle_view&& other) noexcept = default;

  handle_view(const handle_view& other) = default;
  handle_view& operator=(const handle_view& other) = default;

  virtual ~handle_view() = default;

  constexpr explicit operator bool() const noexcept
  {
    return valid();
  }

  constexpr operator value_type() const noexcept
  {
    return value_;
  }

  constexpr bool valid() const noexcept
  {
    return value_ != invalid_value();
  }

  constexpr value_type& value() noexcept
  {
    return value_;
  }

  constexpr const value_type& value() const noexcept
  {
    return value_;
  }

  template <typename V, typename = std::enable_if_t<ice::is_handle_v<V>>>
  constexpr V as() const noexcept
  {
    if constexpr (std::is_pointer_v<value_type> || std::is_pointer_v<V>) {
      return reinterpret_cast<V>(value_);
    } else {
      return static_cast<V>(value_);
    }
  }

protected:
  value_type value_ = invalid_value();
};

template <typename T, auto I>
constexpr bool operator==(const ice::handle_view<T, I>& lhs, const ice::handle_view<T, I>& rhs) noexcept
{
  return lhs.value() == rhs.value();
}

template <typename T, auto I, typename C>
constexpr bool operator!=(const ice::handle_view<T, I>& lhs, const ice::handle_view<T, I>& rhs) noexcept
{
  return !(rhs == lhs);
}

template <typename T, auto I, typename C>
constexpr bool operator==(const ice::handle_view<T, I>& lhs, std::common_type_t<T> value) noexcept
{
  return lhs.value() == value;
}

template <typename T, auto I, typename C>
constexpr bool operator!=(const ice::handle_view<T, I>& lhs, std::common_type_t<T> value) noexcept
{
  return !(lhs == value);
}

template <typename T, auto I, typename C>
constexpr bool operator==(std::common_type_t<T> value, const ice::handle_view<T, I>& rhs) noexcept
{
  return value == rhs.value();
}

template <typename T, auto I, typename C>
constexpr bool operator!=(std::common_type_t<T> value, const ice::handle_view<T, I>& rhs) noexcept
{
  return !(value == rhs);
}

template <typename T, auto I, typename C>
class handle final : public handle_view<T, I> {
public:
  using view = handle_view<T, I>;
  using value_type = typename view::value_type;

  constexpr handle() noexcept = default;

  template <typename V, typename = std::enable_if_t<ice::is_handle_v<V>>>
  constexpr explicit handle(V value) noexcept : view(value)
  {}

  constexpr handle(handle&& other) noexcept : view(other.release()) {}

  handle& operator=(handle&& other) noexcept
  {
    handle(std::move(other)).swap(*this);
    return *this;
  }

  handle(const handle& other) = delete;
  handle& operator=(const handle& other) = delete;

  ~handle()
  {
    reset();
  }

  constexpr void swap(handle& other) noexcept
  {
    const auto value = other.value();
    other.value() = view::value_;
    view::value_ = value;
  }

  constexpr value_type release() noexcept
  {
    const auto value = view::value_;
    view::value_ = view::invalid_value();
    return value;
  }

  void reset() noexcept
  {
    reset(view::invalid_value());
  }

  template <typename V, typename = std::enable_if_t<ice::is_handle_v<V>>>
  void reset(V value) noexcept
  {
    if (view::valid()) {
      C{}(*this);
    }
    if constexpr (std::is_pointer_v<value_type> || std::is_pointer_v<V>) {
      view::value_ = reinterpret_cast<value_type>(value);
    } else {
      view::value_ = static_cast<value_type>(value);
    }
  }
};

}  // namespace ice
