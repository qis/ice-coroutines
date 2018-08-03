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
  using storage_type = detail::result_storage<T>;
  using value_type = typename storage_type::value_type;
  using error_type = typename storage_type::error_type;

  template <typename... Args, std::enable_if_t<std::is_constructible_v<value_type, Args&&...>>* = nullptr>
  result(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args&&...>) : storage_(std::forward<Args>(args)...)
  {}

  explicit result(error_type error) noexcept : storage_(error) {}

  result(result&& other) = default;
  result(const result& other) = default;
  result& operator=(result&& other) = default;
  result& operator=(const result& other) = default;

private:
  storage_type storage_;
};

}  // namespace ice

#if 0

// expected - An implementation of std::expected with extensions
// Written in 2017 by Simon Brand (@TartanLlama)
//
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to the
// public domain worldwide. This software is distributed without any warranty.
//
// You should have received a copy of the CC0 Public Domain Dedication
// along with this software. If not, see
// <http://creativecommons.org/publicdomain/zero/1.0/>.

#  include <exception>
#  include <functional>
#  include <type_traits>
#  include <utility>

namespace tl {

template <typename T, typename E>
class expected;

class monostate {};

struct in_place_t {
  explicit in_place_t() = default;
};

static constexpr in_place_t in_place{};

template <typename E>
class unexpected {
public:
  static_assert(!std::is_same<E, void>::value, "E must not be void");

  unexpected() = delete;
  constexpr explicit unexpected(const E& e) : m_val(e) {}

  constexpr explicit unexpected(E&& e) : m_val(std::move(e)) {}

  constexpr const E& value() const&
  {
    return m_val;
  }

  constexpr E& value() &
  {
    return m_val;
  }

  constexpr E&& value() &&
  {
    return std::move(m_val);
  }

  constexpr const E&& value() const&&
  {
    return std::move(m_val);
  }

private:
  E m_val;
};

template <typename E>
constexpr bool operator==(const unexpected<E>& lhs, const unexpected<E>& rhs)
{
  return lhs.value() == rhs.value();
}

template <typename E>
constexpr bool operator!=(const unexpected<E>& lhs, const unexpected<E>& rhs)
{
  return lhs.value() != rhs.value();
}

template <typename E>
constexpr bool operator<(const unexpected<E>& lhs, const unexpected<E>& rhs)
{
  return lhs.value() < rhs.value();
}

template <typename E>
constexpr bool operator<=(const unexpected<E>& lhs, const unexpected<E>& rhs)
{
  return lhs.value() <= rhs.value();
}

template <typename E>
constexpr bool operator>(const unexpected<E>& lhs, const unexpected<E>& rhs)
{
  return lhs.value() > rhs.value();
}

template <typename E>
constexpr bool operator>=(const unexpected<E>& lhs, const unexpected<E>& rhs)
{
  return lhs.value() >= rhs.value();
}

template <typename E>
unexpected<typename std::decay_t<E>> make_unexpected(E&& e)
{
  return unexpected<typename std::decay_t<E>>(std::forward<E>(e));
}

struct unexpect_t {
  unexpect_t() = default;
};

static constexpr unexpect_t unexpect{};

namespace detail {

template <typename T>
struct is_expected_impl : std::false_type {};

template <typename T, typename E>
struct is_expected_impl<expected<T, E>> : std::true_type {};

template <typename T>
using is_expected = is_expected_impl<std::decay_t<T>>;

template <typename T, typename E, typename U>
using expected_enable_forward_value = std::enable_if_t<
  std::is_constructible_v<T, U&&> && !std::is_same_v<std::decay_t<U>, in_place_t> &&
  !std::is_same_v<expected<T, E>, std::decay_t<U>> && !std::is_same_v<unexpected<E>, std::decay_t<U>>>;

template <typename T, typename E, typename U, typename G, typename UR, typename GR>
using expected_enable_from_other = std::enable_if_t<
  std::is_constructible_v<T, UR> && std::is_constructible_v<T, GR> && !std::is_constructible_v<T, expected<U, G>&> &&
  !std::is_constructible_v<T, expected<U, G>&&> && !std::is_constructible_v<T, const expected<U, G>&> &&
  !std::is_constructible_v<T, const expected<U, G>&&> && !std::is_convertible_v<expected<U, G>&, T> &&
  !std::is_convertible_v<expected<U, G>&&, T> && !std::is_convertible_v<const expected<U, G>&, T> &&
  !std::is_convertible_v<const expected<U, G>&&, T>>;

template <typename T, typename U>
using is_void_or = std::conditional_t<std::is_void_v<T>, std::true_type, U>;

template <typename T>
using is_copy_constructible_or_void = is_void_or<T, std::is_copy_constructible<T>>;

template <typename T>
using is_move_constructible_or_void = is_void_or<T, std::is_move_constructible<T>>;

}  // namespace detail

namespace detail {
struct no_init_t {};
static constexpr no_init_t no_init{};

template <typename T, typename E, bool = std::is_trivially_destructible_v<T>, bool = std::is_trivially_destructible_v<E>>
struct expected_storage_base {
  constexpr expected_storage_base() : m_val(T{}), m_has_val(true) {}
  constexpr expected_storage_base(no_init_t) : m_has_val(false) {}

  template <typename... Args, std::enable_if_t<std::is_constructible_v<T, Args&&...>>* = nullptr>
  constexpr expected_storage_base(in_place_t, Args&&... args) : m_val(std::forward<Args>(args)...), m_has_val(true)
  {}

  template <
    class U,
    class... Args,
    std::enable_if_t<std::is_constructible_v<T, std::initializer_list<U>&, Args&&...>>* = nullptr>
  constexpr expected_storage_base(in_place_t, std::initializer_list<U> il, Args&&... args) :
    m_val(il, std::forward<Args>(args)...), m_has_val(true)
  {}
  template <typename... Args, std::enable_if_t<std::is_constructible_v<E, Args&&...>>* = nullptr>
  constexpr explicit expected_storage_base(unexpect_t, Args&&... args) :
    m_unexpect(std::forward<Args>(args)...), m_has_val(false)
  {}

  template <
    class U,
    class... Args,
    std::enable_if_t<std::is_constructible_v<E, std::initializer_list<U>&, Args&&...>>* = nullptr>
  constexpr explicit expected_storage_base(unexpect_t, std::initializer_list<U> il, Args&&... args) :
    m_unexpect(il, std::forward<Args>(args)...), m_has_val(false)
  {}

  ~expected_storage_base()
  {
    if (m_has_val) {
      m_val.~T();
    } else {
      m_unexpect.~unexpected<E>();
    }
  }
  union {
    T m_val;
    unexpected<E> m_unexpect;
  };
  bool m_has_val;
};

template <typename T, typename E>
struct expected_storage_base<T, E, true, true> {
  constexpr expected_storage_base() : m_val(T{}), m_has_val(true) {}
  constexpr expected_storage_base(no_init_t) : m_has_val(false) {}

  template <typename... Args, std::enable_if_t<std::is_constructible_v<T, Args&&...>>* = nullptr>
  constexpr expected_storage_base(in_place_t, Args&&... args) : m_val(std::forward<Args>(args)...), m_has_val(true)
  {}

  template <
    class U,
    class... Args,
    std::enable_if_t<std::is_constructible_v<T, std::initializer_list<U>&, Args&&...>>* = nullptr>
  constexpr expected_storage_base(in_place_t, std::initializer_list<U> il, Args&&... args) :
    m_val(il, std::forward<Args>(args)...), m_has_val(true)
  {}
  template <typename... Args, std::enable_if_t<std::is_constructible_v<E, Args&&...>>* = nullptr>
  constexpr explicit expected_storage_base(unexpect_t, Args&&... args) :
    m_unexpect(std::forward<Args>(args)...), m_has_val(false)
  {}

  template <
    class U,
    class... Args,
    std::enable_if_t<std::is_constructible_v<E, std::initializer_list<U>&, Args&&...>>* = nullptr>
  constexpr explicit expected_storage_base(unexpect_t, std::initializer_list<U> il, Args&&... args) :
    m_unexpect(il, std::forward<Args>(args)...), m_has_val(false)
  {}

  ~expected_storage_base() = default;
  union {
    T m_val;
    unexpected<E> m_unexpect;
  };
  bool m_has_val;
};

template <typename T, typename E>
struct expected_storage_base<T, E, true, false> {
  constexpr expected_storage_base() : m_val(T{}), m_has_val(true) {}
  constexpr expected_storage_base(no_init_t) : m_has_val(false) {}

  template <typename... Args, std::enable_if_t<std::is_constructible_v<T, Args&&...>>* = nullptr>
  constexpr expected_storage_base(in_place_t, Args&&... args) : m_val(std::forward<Args>(args)...), m_has_val(true)
  {}

  template <
    class U,
    class... Args,
    std::enable_if_t<std::is_constructible_v<T, std::initializer_list<U>&, Args&&...>>* = nullptr>
  constexpr expected_storage_base(in_place_t, std::initializer_list<U> il, Args&&... args) :
    m_val(il, std::forward<Args>(args)...), m_has_val(true)
  {}
  template <typename... Args, std::enable_if_t<std::is_constructible_v<E, Args&&...>>* = nullptr>
  constexpr explicit expected_storage_base(unexpect_t, Args&&... args) :
    m_unexpect(std::forward<Args>(args)...), m_has_val(false)
  {}

  template <
    class U,
    class... Args,
    std::enable_if_t<std::is_constructible_v<E, std::initializer_list<U>&, Args&&...>>* = nullptr>
  constexpr explicit expected_storage_base(unexpect_t, std::initializer_list<U> il, Args&&... args) :
    m_unexpect(il, std::forward<Args>(args)...), m_has_val(false)
  {}

  ~expected_storage_base()
  {
    if (!m_has_val) {
      m_unexpect.~unexpected<E>();
    }
  }

  union {
    T m_val;
    unexpected<E> m_unexpect;
  };
  bool m_has_val;
};

template <typename T, typename E>
struct expected_storage_base<T, E, false, true> {
  constexpr expected_storage_base() : m_val(T{}), m_has_val(true) {}
  constexpr expected_storage_base(no_init_t) : m_has_val(false) {}

  template <typename... Args, std::enable_if_t<std::is_constructible_v<T, Args&&...>>* = nullptr>
  constexpr expected_storage_base(in_place_t, Args&&... args) : m_val(std::forward<Args>(args)...), m_has_val(true)
  {}

  template <
    class U,
    class... Args,
    std::enable_if_t<std::is_constructible_v<T, std::initializer_list<U>&, Args&&...>>* = nullptr>
  constexpr expected_storage_base(in_place_t, std::initializer_list<U> il, Args&&... args) :
    m_val(il, std::forward<Args>(args)...), m_has_val(true)
  {}
  template <typename... Args, std::enable_if_t<std::is_constructible_v<E, Args&&...>>* = nullptr>
  constexpr explicit expected_storage_base(unexpect_t, Args&&... args) :
    m_unexpect(std::forward<Args>(args)...), m_has_val(false)
  {}

  template <
    class U,
    class... Args,
    std::enable_if_t<std::is_constructible_v<E, std::initializer_list<U>&, Args&&...>>* = nullptr>
  constexpr explicit expected_storage_base(unexpect_t, std::initializer_list<U> il, Args&&... args) :
    m_unexpect(il, std::forward<Args>(args)...), m_has_val(false)
  {}

  ~expected_storage_base()
  {
    if (m_has_val) {
      m_val.~T();
    }
  }
  union {
    T m_val;
    unexpected<E> m_unexpect;
  };
  bool m_has_val;
};

template <typename E>
struct expected_storage_base<void, E, false, true> {
  constexpr expected_storage_base() : m_has_val(true) {}
  constexpr expected_storage_base(no_init_t) : m_has_val(false) {}

  constexpr expected_storage_base(in_place_t) : m_has_val(true) {}

  template <typename... Args, std::enable_if_t<std::is_constructible_v<E, Args&&...>>* = nullptr>
  constexpr explicit expected_storage_base(unexpect_t, Args&&... args) :
    m_unexpect(std::forward<Args>(args)...), m_has_val(false)
  {}

  template <
    class U,
    class... Args,
    std::enable_if_t<std::is_constructible_v<E, std::initializer_list<U>&, Args&&...>>* = nullptr>
  constexpr explicit expected_storage_base(unexpect_t, std::initializer_list<U> il, Args&&... args) :
    m_unexpect(il, std::forward<Args>(args)...), m_has_val(false)
  {}

  ~expected_storage_base() = default;
  struct dummy {};
  union {
    dummy m_val;
    unexpected<E> m_unexpect;
  };
  bool m_has_val;
};

template <typename E>
struct expected_storage_base<void, E, false, false> {
  constexpr expected_storage_base() : m_has_val(true) {}
  constexpr expected_storage_base(no_init_t) : m_has_val(false) {}

  constexpr expected_storage_base(in_place_t) : m_has_val(true) {}

  template <typename... Args, std::enable_if_t<std::is_constructible_v<E, Args&&...>>* = nullptr>
  constexpr explicit expected_storage_base(unexpect_t, Args&&... args) :
    m_unexpect(std::forward<Args>(args)...), m_has_val(false)
  {}

  template <
    class U,
    class... Args,
    std::enable_if_t<std::is_constructible_v<E, std::initializer_list<U>&, Args&&...>>* = nullptr>
  constexpr explicit expected_storage_base(unexpect_t, std::initializer_list<U> il, Args&&... args) :
    m_unexpect(il, std::forward<Args>(args)...), m_has_val(false)
  {}

  ~expected_storage_base()
  {
    if (!m_has_val) {
      m_unexpect.~unexpected<E>();
    }
  }

  struct dummy {};
  union {
    dummy m_val;
    unexpected<E> m_unexpect;
  };
  bool m_has_val;
};

template <typename T, typename E>
struct expected_operations_base : expected_storage_base<T, E> {
  using expected_storage_base<T, E>::expected_storage_base;

  template <typename... Args>
  void construct(Args&&... args) noexcept
  {
    new (std::addressof(this->m_val)) T(std::forward<Args>(args)...);
    this->m_has_val = true;
  }

  template <typename Rhs>
  void construct_with(Rhs&& rhs) noexcept
  {
    new (std::addressof(this->m_val)) T(std::forward<Rhs>(rhs).get());
    this->m_has_val = true;
  }

  template <typename... Args>
  void construct_error(Args&&... args) noexcept
  {
    new (std::addressof(this->m_unexpect)) unexpected<E>(std::forward<Args>(args)...);
    this->m_has_val = false;
  }

  template <typename U = T, std::enable_if_t<std::is_nothrow_copy_constructible_v<U>>* = nullptr>
  void assign(const expected_operations_base& rhs) noexcept
  {
    if (!this->m_has_val && rhs.m_has_val) {
      geterr().~unexpected<E>();
      construct(rhs.get());
    } else {
      assign_common(rhs);
    }
  }

  template <
    class U = T,
    std::enable_if_t<!std::is_nothrow_copy_constructible_v<U> && std::is_nothrow_move_constructible_v<U>>* = nullptr>
  void assign(const expected_operations_base& rhs) noexcept
  {
    if (!this->m_has_val && rhs.m_has_val) {
      T tmp = rhs.get();
      geterr().~unexpected<E>();
      construct(std::move(tmp));
    } else {
      assign_common(rhs);
    }
  }

  template <
    class U = T,
    std::enable_if_t<!std::is_nothrow_copy_constructible_v<U> && !std::is_nothrow_move_constructible_v<U>>* = nullptr>
  void assign(const expected_operations_base& rhs)
  {
    if (!this->m_has_val && rhs.m_has_val) {
      auto tmp = std::move(geterr());
      geterr().~unexpected<E>();

      try {
        construct(rhs.get());
      }
      catch (...) {
        geterr() = std::move(tmp);
        throw;
      }
    } else {
      assign_common(rhs);
    }
  }

  template <typename U = T, std::enable_if_t<std::is_nothrow_move_constructible_v<U>>* = nullptr>
  void assign(expected_operations_base&& rhs) noexcept
  {
    if (!this->m_has_val && rhs.m_has_val) {
      geterr().~unexpected<E>();
      construct(std::move(rhs).get());
    } else {
      assign_common(rhs);
    }
  }

  template <typename U = T, std::enable_if_t<!std::is_nothrow_move_constructible_v<U>>* = nullptr>
  void assign(expected_operations_base&& rhs)
  {
    if (!this->m_has_val && rhs.m_has_val) {
      auto tmp = std::move(geterr());
      geterr().~unexpected<E>();
      try {
        construct(std::move(rhs).get());
      }
      catch (...) {
        geterr() = std::move(tmp);
        throw;
      }
    } else {
      assign_common(rhs);
    }
  }

  template <typename Rhs>
  void assign_common(Rhs&& rhs)
  {
    if (this->m_has_val) {
      if (rhs.m_has_val) {
        get() = std::forward<Rhs>(rhs).get();
      } else {
        get().~T();
        construct_error(std::forward<Rhs>(rhs).geterr());
      }
    } else {
      if (!rhs.m_has_val) {
        geterr() = std::forward<Rhs>(rhs).geterr();
      }
    }
  }

  bool has_value() const
  {
    return this->m_has_val;
  }

  constexpr T& get() &
  {
    return this->m_val;
  }
  constexpr const T& get() const&
  {
    return this->m_val;
  }
  constexpr T&& get() &&
  {
    return std::move(this->m_val);
  }
#  ifndef TL_EXPECTED_NO_CONSTRR
  constexpr const T&& get() const&&
  {
    return std::move(this->m_val);
  }
#  endif

  constexpr unexpected<E>& geterr() &
  {
    return this->m_unexpect;
  }
  constexpr const unexpected<E>& geterr() const&
  {
    return this->m_unexpect;
  }
  constexpr unexpected<E>&& geterr() &&
  {
    std::move(this->m_unexpect);
  }
#  ifndef TL_EXPECTED_NO_CONSTRR
  constexpr const unexpected<E>&& geterr() const&&
  {
    return std::move(this->m_unexpect);
  }
#  endif
};

template <typename E>
struct expected_operations_base<void, E> : expected_storage_base<void, E> {
  using expected_storage_base<void, E>::expected_storage_base;

  template <typename... Args>
  void construct() noexcept
  {
    this->m_has_val = true;
  }

  template <typename Rhs>
  void construct_with(Rhs&&) noexcept
  {
    this->m_has_val = true;
  }

  template <typename... Args>
  void construct_error(Args&&... args) noexcept
  {
    new (std::addressof(this->m_unexpect)) unexpected<E>(std::forward<Args>(args)...);
    this->m_has_val = false;
  }

  template <typename Rhs>
  void assign(Rhs&& rhs) noexcept
  {
    if (!this->m_has_val) {
      if (rhs.m_has_val) {
        geterr().~unexpected<E>();
        construct();
      } else {
        geterr() = std::forward<Rhs>(rhs).geterr();
      }
    } else {
      if (!rhs.m_has_val) {
        construct_error(std::forward<Rhs>(rhs).geterr());
      }
    }
  }

  bool has_value() const
  {
    return this->m_has_val;
  }

  constexpr unexpected<E>& geterr() &
  {
    return this->m_unexpect;
  }
  constexpr const unexpected<E>& geterr() const&
  {
    return this->m_unexpect;
  }
  constexpr unexpected<E>&& geterr() &&
  {
    std::move(this->m_unexpect);
  }
#  ifndef TL_EXPECTED_NO_CONSTRR
  constexpr const unexpected<E>&& geterr() const&&
  {
    return std::move(this->m_unexpect);
  }
#  endif
};

template <
  class T,
  class E,
  bool = is_void_or<T, std::is_trivially_copy_constructible<T>>::value&& std::is_trivially_copy_constructible_v<E>>
struct expected_copy_base : expected_operations_base<T, E> {
  using expected_operations_base<T, E>::expected_operations_base;
};

template <typename T, typename E>
struct expected_copy_base<T, E, false> : expected_operations_base<T, E> {
  using expected_operations_base<T, E>::expected_operations_base;

  expected_copy_base() = default;
  expected_copy_base(const expected_copy_base& rhs) : expected_operations_base<T, E>(no_init)
  {
    if (rhs.has_value()) {
      this->construct_with(rhs);
    } else {
      this->construct_error(rhs.geterr());
    }
  }

  expected_copy_base(expected_copy_base&& rhs) = default;
  expected_copy_base& operator=(const expected_copy_base& rhs) = default;
  expected_copy_base& operator=(expected_copy_base&& rhs) = default;
};

template <
  class T,
  class E,
  bool = is_void_or<T, std::is_trivially_move_constructible<T>>::value&& std::is_trivially_move_constructible<E>::value>
struct expected_move_base : expected_copy_base<T, E> {
  using expected_copy_base<T, E>::expected_copy_base;
};

template <typename T, typename E>
struct expected_move_base<T, E, false> : expected_copy_base<T, E> {
  using expected_copy_base<T, E>::expected_copy_base;

  expected_move_base() = default;
  expected_move_base(const expected_move_base& rhs) = default;

  expected_move_base(expected_move_base&& rhs) noexcept(std::is_nothrow_move_constructible_v<T>) :
    expected_copy_base<T, E>(no_init)
  {
    if (rhs.has_value()) {
      this->construct_with(std::move(rhs));
    } else {
      this->construct_error(std::move(rhs.geterr()));
    }
  }
  expected_move_base& operator=(const expected_move_base& rhs) = default;
  expected_move_base& operator=(expected_move_base&& rhs) = default;
};

template <
  class T,
  class E,
  bool = is_void_or<
    T,
    std::conjunction<
      std::is_trivially_copy_assignable<T>,
      std::is_trivially_copy_constructible<T>,
      std::is_trivially_destructible<T>>>::value&& std::is_trivially_copy_assignable_v<E>&&
    std::is_trivially_copy_constructible_v<E>&& std::is_trivially_destructible_v<E>>
struct expected_copy_assign_base : expected_move_base<T, E> {
  using expected_move_base<T, E>::expected_move_base;
};

template <typename T, typename E>
struct expected_copy_assign_base<T, E, false> : expected_move_base<T, E> {
  using expected_move_base<T, E>::expected_move_base;

  expected_copy_assign_base() = default;
  expected_copy_assign_base(const expected_copy_assign_base& rhs) = default;

  expected_copy_assign_base(expected_copy_assign_base&& rhs) = default;
  expected_copy_assign_base& operator=(const expected_copy_assign_base& rhs)
  {
    this->assign(rhs);
    return *this;
  }
  expected_copy_assign_base& operator=(expected_copy_assign_base&& rhs) = default;
};

template <
  class T,
  class E,
  bool = is_void_or<
    T,
    std::conjunction<
      std::is_trivially_destructible<T>,
      std::is_trivially_move_constructible<T>,
      std::is_trivially_move_assignable<T>>>::value&& std::is_trivially_destructible_v<E>&&
    std::is_trivially_move_constructible_v<E>&& std::is_trivially_move_assignable_v<E>>
struct expected_move_assign_base : expected_copy_assign_base<T, E> {
  using expected_copy_assign_base<T, E>::expected_copy_assign_base;
};

template <typename T, typename E>
struct expected_move_assign_base<T, E, false> : expected_copy_assign_base<T, E> {
  using expected_copy_assign_base<T, E>::expected_copy_assign_base;

  expected_move_assign_base() = default;
  expected_move_assign_base(const expected_move_assign_base& rhs) = default;

  expected_move_assign_base(expected_move_assign_base&& rhs) = default;

  expected_move_assign_base& operator=(const expected_move_assign_base& rhs) = default;

  expected_move_assign_base& operator=(expected_move_assign_base&& rhs) noexcept(
    std::is_nothrow_move_constructible_v<T>&& std::is_nothrow_move_assignable_v<T>)
  {
    this->assign(std::move(rhs));
    return *this;
  }
};

template <
  class T,
  class E,
  bool EnableCopy = (is_copy_constructible_or_void<T>::value && std::is_copy_constructible_v<E>),
  bool EnableMove = (is_move_constructible_or_void<T>::value && std::is_move_constructible_v<E>)>
struct expected_delete_ctor_base {
  expected_delete_ctor_base() = default;
  expected_delete_ctor_base(const expected_delete_ctor_base&) = default;
  expected_delete_ctor_base(expected_delete_ctor_base&&) noexcept = default;
  expected_delete_ctor_base& operator=(const expected_delete_ctor_base&) = default;
  expected_delete_ctor_base& operator=(expected_delete_ctor_base&&) noexcept = default;
};

template <typename T, typename E>
struct expected_delete_ctor_base<T, E, true, false> {
  expected_delete_ctor_base() = default;
  expected_delete_ctor_base(const expected_delete_ctor_base&) = default;
  expected_delete_ctor_base(expected_delete_ctor_base&&) noexcept = delete;
  expected_delete_ctor_base& operator=(const expected_delete_ctor_base&) = default;
  expected_delete_ctor_base& operator=(expected_delete_ctor_base&&) noexcept = default;
};

template <typename T, typename E>
struct expected_delete_ctor_base<T, E, false, true> {
  expected_delete_ctor_base() = default;
  expected_delete_ctor_base(const expected_delete_ctor_base&) = delete;
  expected_delete_ctor_base(expected_delete_ctor_base&&) noexcept = default;
  expected_delete_ctor_base& operator=(const expected_delete_ctor_base&) = default;
  expected_delete_ctor_base& operator=(expected_delete_ctor_base&&) noexcept = default;
};

template <typename T, typename E>
struct expected_delete_ctor_base<T, E, false, false> {
  expected_delete_ctor_base() = default;
  expected_delete_ctor_base(const expected_delete_ctor_base&) = delete;
  expected_delete_ctor_base(expected_delete_ctor_base&&) noexcept = delete;
  expected_delete_ctor_base& operator=(const expected_delete_ctor_base&) = default;
  expected_delete_ctor_base& operator=(expected_delete_ctor_base&&) noexcept = default;
};

template <
  class T,
  class E,
  bool EnableCopy =
    (std::is_copy_constructible_v<T> && std::is_copy_constructible_v<E> && std::is_copy_assignable_v<T> &&
     std::is_copy_assignable_v<E>),
  bool EnableMove =
    (std::is_move_constructible_v<T> && std::is_move_constructible_v<E> && std::is_move_assignable_v<T> &&
     std::is_move_assignable_v<E>)>
struct expected_delete_assign_base {
  expected_delete_assign_base() = default;
  expected_delete_assign_base(const expected_delete_assign_base&) = default;
  expected_delete_assign_base(expected_delete_assign_base&&) noexcept = default;
  expected_delete_assign_base& operator=(const expected_delete_assign_base&) = default;
  expected_delete_assign_base& operator=(expected_delete_assign_base&&) noexcept = default;
};

template <typename T, typename E>
struct expected_delete_assign_base<T, E, true, false> {
  expected_delete_assign_base() = default;
  expected_delete_assign_base(const expected_delete_assign_base&) = default;
  expected_delete_assign_base(expected_delete_assign_base&&) noexcept = default;
  expected_delete_assign_base& operator=(const expected_delete_assign_base&) = default;
  expected_delete_assign_base& operator=(expected_delete_assign_base&&) noexcept = delete;
};

template <typename T, typename E>
struct expected_delete_assign_base<T, E, false, true> {
  expected_delete_assign_base() = default;
  expected_delete_assign_base(const expected_delete_assign_base&) = default;
  expected_delete_assign_base(expected_delete_assign_base&&) noexcept = default;
  expected_delete_assign_base& operator=(const expected_delete_assign_base&) = delete;
  expected_delete_assign_base& operator=(expected_delete_assign_base&&) noexcept = default;
};

template <typename T, typename E>
struct expected_delete_assign_base<T, E, false, false> {
  expected_delete_assign_base() = default;
  expected_delete_assign_base(const expected_delete_assign_base&) = default;
  expected_delete_assign_base(expected_delete_assign_base&&) noexcept = default;
  expected_delete_assign_base& operator=(const expected_delete_assign_base&) = delete;
  expected_delete_assign_base& operator=(expected_delete_assign_base&&) noexcept = delete;
};

struct default_constructor_tag {
  explicit constexpr default_constructor_tag() = default;
};

template <typename T, typename E, bool Enable = std::is_default_constructible_v<T> || std::is_void_v<T>>
struct expected_default_ctor_base {
  constexpr expected_default_ctor_base() noexcept = default;
  constexpr expected_default_ctor_base(expected_default_ctor_base const&) noexcept = default;
  constexpr expected_default_ctor_base(expected_default_ctor_base&&) noexcept = default;
  expected_default_ctor_base& operator=(expected_default_ctor_base const&) noexcept = default;
  expected_default_ctor_base& operator=(expected_default_ctor_base&&) noexcept = default;

  constexpr explicit expected_default_ctor_base(default_constructor_tag) {}
};

template <typename T, typename E>
struct expected_default_ctor_base<T, E, false> {
  constexpr expected_default_ctor_base() noexcept = delete;
  constexpr expected_default_ctor_base(expected_default_ctor_base const&) noexcept = default;
  constexpr expected_default_ctor_base(expected_default_ctor_base&&) noexcept = default;
  expected_default_ctor_base& operator=(expected_default_ctor_base const&) noexcept = default;
  expected_default_ctor_base& operator=(expected_default_ctor_base&&) noexcept = default;

  constexpr explicit expected_default_ctor_base(default_constructor_tag) {}
};
}  // namespace detail

template <typename E>
class bad_expected_access : public std::exception {
public:
  explicit bad_expected_access(E e) : m_val(std::move(e)) {}

  virtual const char* what() const noexcept override
  {
    return "Bad expected access";
  }

  const E& error() const&
  {
    return m_val;
  }
  E& error() &
  {
    return m_val;
  }
  const E&& error() const&&
  {
    return std::move(m_val);
  }
  E&& error() &&
  {
    return std::move(m_val);
  }

private:
  E m_val;
};

template <typename T, typename E>
class expected : private detail::expected_move_assign_base<T, E>,
                 private detail::expected_delete_ctor_base<T, E>,
                 private detail::expected_delete_assign_base<T, E>,
                 private detail::expected_default_ctor_base<T, E> {
  static_assert(!std::is_reference_v<T>, "T must not be a reference");
  static_assert(!std::is_same_v<T, std::remove_cv<in_place_t>>, "T must not be in_place_t");
  static_assert(!std::is_same_v<T, std::remove_cv<unexpect_t>>, "T must not be unexpect_t");
  static_assert(!std::is_same_v<T, std::remove_cv<unexpected<E>>>, "T must not be unexpected<E>");
  static_assert(!std::is_reference_v<E>, "E must not be a reference");

  T* valptr()
  {
    return std::addressof(this->m_val);
  }
  unexpected<E>* errptr()
  {
    return std::addressof(this->m_unexpect);
  }

  template <typename U = T, std::enable_if_t<!std::is_void_v<U>>* = nullptr>
  U& val()
  {
    return this->m_val;
  }
  unexpected<E>& err()
  {
    return this->m_unexpect;
  }

  template <typename U = T, std::enable_if_t<!std::is_void_v<U>>* = nullptr>
  const U& val() const
  {
    return this->m_val;
  }
  const unexpected<E>& err() const
  {
    return this->m_unexpect;
  }

  using impl_base = detail::expected_move_assign_base<T, E>;
  using ctor_base = detail::expected_default_ctor_base<T, E>;

public:
  typedef T value_type;
  typedef E error_type;
  typedef unexpected<E> unexpected_type;

  constexpr expected() = default;
  constexpr expected(const expected& rhs) = default;
  constexpr expected(expected&& rhs) = default;
  expected& operator=(const expected& rhs) = default;
  expected& operator=(expected&& rhs) = default;

  template <typename... Args, std::enable_if_t<std::is_constructible_v<T, Args&&...>>* = nullptr>
  constexpr expected(in_place_t, Args&&... args) :
    impl_base(in_place, std::forward<Args>(args)...), ctor_base(detail::default_constructor_tag{})
  {}

  template <
    class U,
    class... Args,
    std::enable_if_t<std::is_constructible_v<T, std::initializer_list<U>&, Args&&...>>* = nullptr>
  constexpr expected(in_place_t, std::initializer_list<U> il, Args&&... args) :
    impl_base(in_place, il, std::forward<Args>(args)...), ctor_base(detail::default_constructor_tag{})
  {}

  template <
    class G = E,
    std::enable_if_t<std::is_constructible_v<E, const G&>>* = nullptr,
    std::enable_if_t<!std::is_convertible_v<const G&, E>>* = nullptr>
  explicit constexpr expected(const unexpected<G>& e) :
    impl_base(unexpect, e.value()), ctor_base(detail::default_constructor_tag{})
  {}

  template <
    class G = E,
    std::enable_if_t<std::is_constructible_v<E, const G&>>* = nullptr,
    std::enable_if_t<std::is_convertible_v<const G&, E>>* = nullptr>
  constexpr expected(unexpected<G> const& e) :
    impl_base(unexpect, e.value()), ctor_base(detail::default_constructor_tag{})
  {}

  template <
    class G = E,
    std::enable_if_t<std::is_constructible_v<E, G&&>>* = nullptr,
    std::enable_if_t<!std::is_convertible_v<G&&, E>>* = nullptr>
  explicit constexpr expected(unexpected<G>&& e) noexcept(std::is_nothrow_constructible_v<E, G&&>) :
    impl_base(unexpect, std::move(e.value())), ctor_base(detail::default_constructor_tag{})
  {}

  template <
    class G = E,
    std::enable_if_t<std::is_constructible_v<E, G&&>>* = nullptr,
    std::enable_if_t<std::is_convertible_v<G&&, E>>* = nullptr>
  constexpr expected(unexpected<G>&& e) noexcept(std::is_nothrow_constructible_v<E, G&&>) :
    impl_base(unexpect, std::move(e.value())), ctor_base(detail::default_constructor_tag{})
  {}

  template <typename... Args, std::enable_if_t<std::is_constructible_v<E, Args&&...>>* = nullptr>
  constexpr explicit expected(unexpect_t, Args&&... args) :
    impl_base(unexpect, std::forward<Args>(args)...), ctor_base(detail::default_constructor_tag{})
  {}

  template <
    class U,
    class... Args,
    std::enable_if_t<std::is_constructible_v<E, std::initializer_list<U>&, Args&&...>>* = nullptr>
  constexpr explicit expected(unexpect_t, std::initializer_list<U> il, Args&&... args) :
    impl_base(unexpect, il, std::forward<Args>(args)...), ctor_base(detail::default_constructor_tag{})
  {}

  template <
    class U,
    class G,
    std::enable_if_t<!(std::is_convertible_v<U const&, T> && std::is_convertible_v<G const&, E>)>* = nullptr,
    detail::expected_enable_from_other<T, E, U, G, const U&, const G&>* = nullptr>
  explicit constexpr expected(const expected<U, G>& rhs) : ctor_base(detail::default_constructor_tag{})
  {
    if (rhs.has_value()) {
      ::new (valptr()) T(*rhs);
    } else {
      ::new (errptr()) unexpected_type(unexpected<E>(rhs.error()));
    }
  }

  template <
    class U,
    class G,
    std::enable_if_t<(std::is_convertible_v<U const&, T> && std::is_convertible_v<G const&, E>)>* = nullptr,
    detail::expected_enable_from_other<T, E, U, G, const U&, const G&>* = nullptr>
  constexpr expected(const expected<U, G>& rhs) : ctor_base(detail::default_constructor_tag{})
  {
    if (rhs.has_value()) {
      ::new (valptr()) T(*rhs);
    } else {
      ::new (errptr()) unexpected_type(unexpected<E>(rhs.error()));
    }
  }

  template <
    class U,
    class G,
    std::enable_if_t<!(std::is_convertible_v<U&&, T> && std::is_convertible_v<G&&, E>)>* = nullptr,
    detail::expected_enable_from_other<T, E, U, G, U&&, G&&>* = nullptr>
  explicit constexpr expected(expected<U, G>&& rhs) : ctor_base(detail::default_constructor_tag{})
  {
    if (rhs.has_value()) {
      ::new (valptr()) T(std::move(*rhs));
    } else {
      ::new (errptr()) unexpected_type(unexpected<E>(std::move(rhs.error())));
    }
  }

  template <
    class U,
    class G,
    std::enable_if_t<(std::is_convertible_v<U&&, T> && std::is_convertible_v<G&&, E>)>* = nullptr,
    detail::expected_enable_from_other<T, E, U, G, U&&, G&&>* = nullptr>
  constexpr expected(expected<U, G>&& rhs) : ctor_base(detail::default_constructor_tag{})
  {
    if (rhs.has_value()) {
      ::new (valptr()) T(std::move(*rhs));
    } else {
      ::new (errptr()) unexpected_type(unexpected<E>(std::move(rhs.error())));
    }
  }

  template <
    class U = T,
    std::enable_if_t<!std::is_convertible_v<U&&, T>>* = nullptr,
    detail::expected_enable_forward_value<T, E, U>* = nullptr>
  explicit constexpr expected(U&& v) : expected(in_place, std::forward<U>(v))
  {}

  template <
    class U = T,
    std::enable_if_t<std::is_convertible_v<U&&, T>>* = nullptr,
    detail::expected_enable_forward_value<T, E, U>* = nullptr>
  constexpr expected(U&& v) : expected(in_place, std::forward<U>(v))
  {}

  template <
    class U = T,
    class G = T,
    std::enable_if_t<std::is_nothrow_constructible_v<T, U&&>>* = nullptr,
    std::enable_if_t<!std::is_void_v<G>>* = nullptr,
    std::enable_if_t<
      (!std::is_same_v<expected<T, E>, std::decay_t<U>> &&
       !std::conjunction_v<std::is_scalar<T>, std::is_same<T, std::decay_t<U>>> && std::is_constructible_v<T, U> &&
       std::is_assignable_v<G&, U> && std::is_nothrow_move_constructible_v<E>)>* = nullptr>
  expected& operator=(U&& v)
  {
    if (has_value()) {
      val() = std::forward<U>(v);
    } else {
      err().~unexpected<E>();
      ::new (valptr()) T(std::forward<U>(v));
      this->m_has_val = true;
    }

    return *this;
  }

  template <
    class U = T,
    class G = T,
    std::enable_if_t<!std::is_nothrow_constructible_v<T, U&&>>* = nullptr,
    std::enable_if_t<!std::is_void_v<U>>* = nullptr,
    std::enable_if_t<
      (!std::is_same_v<expected<T, E>, std::decay_t<U>> &&
       !std::conjunction_v<std::is_scalar<T>, std::is_same<T, std::decay_t<U>>> && std::is_constructible_v<T, U> &&
       std::is_assignable_v<G&, U> && std::is_nothrow_move_constructible_v<E>)>* = nullptr>
  expected& operator=(U&& v)
  {
    if (has_value()) {
      val() = std::forward<U>(v);
    } else {
      auto tmp = std::move(err());
      err().~unexpected<E>();
      try {
        ::new (valptr()) T(std::move(v));
        this->m_has_val = true;
      }
      catch (...) {
        err() = std::move(tmp);
        throw;
      }
    }

    return *this;
  }

  template <
    class G = E,
    std::enable_if_t<std::is_nothrow_copy_constructible_v<G> && std::is_assignable_v<G&, G>>* = nullptr>
  expected& operator=(const unexpected<G>& rhs)
  {
    if (!has_value()) {
      err() = rhs;
    } else {
      val().~T();
      ::new (errptr()) unexpected<E>(rhs);
      this->m_has_val = false;
    }

    return *this;
  }

  template <
    class G = E,
    std::enable_if_t<std::is_nothrow_move_constructible_v<G> && std::is_move_assignable_v<G>>* = nullptr>
  expected& operator=(unexpected<G>&& rhs) noexcept
  {
    if (!has_value()) {
      err() = std::move(rhs);
    } else {
      val().~T();
      ::new (errptr()) unexpected<E>(std::move(rhs));
      this->m_has_val = false;
    }

    return *this;
  }

  template <typename... Args, std::enable_if_t<std::is_nothrow_constructible_v<T, Args&&...>>* = nullptr>
  void emplace(Args&&... args)
  {
    if (has_value()) {
      val() = T(std::forward<Args>(args)...);
    } else {
      err().~unexpected<E>();
      ::new (valptr()) T(std::forward<Args>(args)...);
      this->m_has_val = true;
    }
  }

  template <typename... Args, std::enable_if_t<!std::is_nothrow_constructible_v<T, Args&&...>>* = nullptr>
  void emplace(Args&&... args)
  {
    if (has_value()) {
      val() = T(std::forward<Args>(args)...);
    } else {
      auto tmp = std::move(err());
      err().~unexpected<E>();

      try {
        ::new (valptr()) T(std::forward<Args>(args)...);
        this->m_has_val = true;
      }
      catch (...) {
        err() = std::move(tmp);
        throw;
      }
    }
  }

  template <
    class U,
    class... Args,
    std::enable_if_t<std::is_nothrow_constructible_v<T, std::initializer_list<U>&, Args&&...>>* = nullptr>
  void emplace(std::initializer_list<U> il, Args&&... args)
  {
    if (has_value()) {
      T t(il, std::forward<Args>(args)...);
      val() = std::move(t);
    } else {
      err().~unexpected<E>();
      ::new (valptr()) T(il, std::forward<Args>(args)...);
      this->m_has_val = true;
    }
  }

  template <
    class U,
    class... Args,
    std::enable_if_t<!std::is_nothrow_constructible_v<T, std::initializer_list<U>&, Args&&...>>* = nullptr>
  void emplace(std::initializer_list<U> il, Args&&... args)
  {
    if (has_value()) {
      T t(il, std::forward<Args>(args)...);
      val() = std::move(t);
    } else {
      auto tmp = std::move(err());
      err().~unexpected<E>();

      try {
        ::new (valptr()) T(il, std::forward<Args>(args)...);
        this->m_has_val = true;
      }
      catch (...) {
        err() = std::move(tmp);
        throw;
      }
    }
  }

  void swap(expected& rhs) noexcept(
    std::is_nothrow_move_constructible_v<T>&& noexcept(swap(std::declval<T&>(), std::declval<T&>())) &&
    std::is_nothrow_move_constructible_v<E> && noexcept(swap(std::declval<E&>(), std::declval<E&>())))
  {
    if (has_value() && rhs.has_value()) {
      using std::swap;
      swap(val(), rhs.val());
    } else if (!has_value() && rhs.has_value()) {
      using std::swap;
      swap(err(), rhs.err());
    } else if (has_value()) {
      auto temp = std::move(rhs.err());
      ::new (rhs.valptr()) T(val());
      ::new (errptr()) unexpected_type(std::move(temp));
      std::swap(this->m_has_val, rhs.m_has_val);
    } else {
      auto temp = std::move(this->err());
      ::new (valptr()) T(rhs.val());
      ::new (errptr()) unexpected_type(std::move(temp));
      std::swap(this->m_has_val, rhs.m_has_val);
    }
  }

  constexpr const T* operator->() const
  {
    return valptr();
  }

  constexpr T* operator->()
  {
    return valptr();
  }

  template <typename U = T, std::enable_if_t<!std::is_void_v<U>>* = nullptr>
  constexpr const U& operator*() const&
  {
    return val();
  }

  template <typename U = T, std::enable_if_t<!std::is_void_v<U>>* = nullptr>
  constexpr U& operator*() &
  {
    return val();
  }

  template <typename U = T, std::enable_if_t<!std::is_void_v<U>>* = nullptr>
  constexpr const U&& operator*() const&&
  {
    return std::move(val());
  }

  template <typename U = T, std::enable_if_t<!std::is_void_v<U>>* = nullptr>
  constexpr U&& operator*() &&
  {
    return std::move(val());
  }

  constexpr bool has_value() const noexcept
  {
    return this->m_has_val;
  }

  constexpr explicit operator bool() const noexcept
  {
    return this->m_has_val;
  }

  template <typename U = T, std::enable_if_t<!std::is_void_v<U>>* = nullptr>
  constexpr const U& value() const&
  {
    if (!has_value())
      throw bad_expected_access<E>(err().value());
    return val();
  }

  template <typename U = T, std::enable_if_t<!std::is_void_v<U>>* = nullptr>
  constexpr U& value() &
  {
    if (!has_value())
      throw bad_expected_access<E>(err().value());
    return val();
  }

  template <typename U = T, std::enable_if_t<!std::is_void_v<U>>* = nullptr>
  constexpr const U&& value() const&&
  {
    if (!has_value())
      throw bad_expected_access<E>(err().value());
    return std::move(val());
  }

  template <typename U = T, std::enable_if_t<!std::is_void_v<U>>* = nullptr>
  constexpr U&& value() &&
  {
    if (!has_value())
      throw bad_expected_access<E>(err().value());
    return std::move(val());
  }

  constexpr const E& error() const&
  {
    return err().value();
  }

  constexpr E& error() &
  {
    return err().value();
  }

  constexpr const E&& error() const&&
  {
    return std::move(err().value());
  }

  constexpr E&& error() &&
  {
    return std::move(err().value());
  }

  template <typename U>
  constexpr T value_or(U&& v) const&
  {
    static_assert(
      std::is_copy_constructible_v<T> && std::is_convertible_v<U&&, T>,
      "T must be copy-constructible and convertible to from U&&");
    return bool(*this) ? **this : static_cast<T>(std::forward<U>(v));
  }

  template <typename U>
  constexpr T value_or(U&& v) &&
  {
    static_assert(
      std::is_move_constructible_v<T> && std::is_convertible_v<U&&, T>,
      "T must be move-constructible and convertible to from U&&");
    return bool(*this) ? std::move(**this) : static_cast<T>(std::forward<U>(v));
  }
};

template <typename T, typename E, typename U, typename F>
constexpr bool operator==(const expected<T, E>& lhs, const expected<U, F>& rhs)
{
  return (lhs.has_value() != rhs.has_value()) ? false : (!lhs.has_value() ? lhs.error() == rhs.error() : *lhs == *rhs);
}

template <typename T, typename E, typename U, typename F>
constexpr bool operator!=(const expected<T, E>& lhs, const expected<U, F>& rhs)
{
  return (lhs.has_value() != rhs.has_value()) ? true : (!lhs.has_value() ? lhs.error() != rhs.error() : *lhs != *rhs);
}

template <typename T, typename E, typename U>
constexpr bool operator==(const expected<T, E>& x, const U& v)
{
  return x.has_value() ? *x == v : false;
}

template <typename T, typename E, typename U>
constexpr bool operator==(const U& v, const expected<T, E>& x)
{
  return x.has_value() ? *x == v : false;
}

template <typename T, typename E, typename U>
constexpr bool operator!=(const expected<T, E>& x, const U& v)
{
  return x.has_value() ? *x != v : true;
}

template <typename T, typename E, typename U>
constexpr bool operator!=(const U& v, const expected<T, E>& x)
{
  return x.has_value() ? *x != v : true;
}

template <typename T, typename E>
constexpr bool operator==(const expected<T, E>& x, const unexpected<E>& e)
{
  return x.has_value() ? false : x.error() == e.value();
}

template <typename T, typename E>
constexpr bool operator==(const unexpected<E>& e, const expected<T, E>& x)
{
  return x.has_value() ? false : x.error() == e.value();
}

template <typename T, typename E>
constexpr bool operator!=(const expected<T, E>& x, const unexpected<E>& e)
{
  return x.has_value() ? true : x.error() != e.value();
}

template <typename T, typename E>
constexpr bool operator!=(const unexpected<E>& e, const expected<T, E>& x)
{
  return x.has_value() ? true : x.error() != e.value();
}

template <
  class T,
  class E,
  std::enable_if_t<std::is_move_constructible_v<T> && std::is_move_constructible_v<E>>* = nullptr>
void swap(expected<T, E>& lhs, expected<T, E>& rhs) noexcept(noexcept(lhs.swap(rhs)))
{
  lhs.swap(rhs);
}

}  // namespace tl

namespace ice {

template <typename T>
using result = tl::expected<T, std::error_code>;

}  // namespace ice
#endif
