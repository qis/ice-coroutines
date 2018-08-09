#pragma once
#include <ice/config.hpp>
#include <ice/handle.hpp>
#include <array>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <cstdarg>
#include <cstdint>

#ifndef ICE_LIKELY
#  ifdef __has_builtin
#    if __has_builtin(__builtin_expect)
#      define ICE_LIKELY(expr) __builtin_expect(expr, 1)
#    endif
#  endif
#endif
#ifndef ICE_LIKELY
#  define ICE_LIKELY(expr) expr
#endif

#ifndef ICE_UNLIKELY
#  ifdef __has_builtin
#    if __has_builtin(__builtin_expect)
#      define ICE_UNLIKELY(expr) __builtin_expect(expr, 0)
#    endif
#  endif
#endif
#ifndef ICE_UNLIKELY
#  define ICE_UNLIKELY(expr) expr
#endif

#ifndef ICE_ALWAYS_INLINE
#  if defined(_MSC_VER) && !defined(__clang__)
#    define ICE_ALWAYS_INLINE __forceinline
#  else
#    define ICE_ALWAYS_INLINE __attribute__((always_inline))
#  endif
#endif

#ifndef ICE_OFFSETOF
#  if defined(_MSC_VER) && !defined(__clang__)
#    define ICE_OFFSETOF(s, m) ((size_t)(&reinterpret_cast<char const volatile&>((((s*)0)->m))))
#  else
#    define ICE_OFFSETOF __builtin_offsetof
#  endif
#endif

namespace ice {

template <typename T, bool = std::is_move_constructible_v<T>>
struct delete_move_constructor {
  delete_move_constructor() = default;
  delete_move_constructor(delete_move_constructor&& other) = default;
  delete_move_constructor(const delete_move_constructor& other) = default;
  delete_move_constructor& operator=(delete_move_constructor&& other) = default;
  delete_move_constructor& operator=(const delete_move_constructor& other) = default;
};

template <typename T>
struct delete_move_constructor<T, false> {
  delete_move_constructor() = default;
  delete_move_constructor(delete_move_constructor&& other) = delete;
  delete_move_constructor(const delete_move_constructor& other) = default;
  delete_move_constructor& operator=(delete_move_constructor&& other) = default;
  delete_move_constructor& operator=(const delete_move_constructor& other) = default;
};

template <typename T, bool = std::is_copy_constructible_v<T>>
struct delete_copy_constructor {
  delete_copy_constructor() = default;
  delete_copy_constructor(delete_copy_constructor&& other) = default;
  delete_copy_constructor(const delete_copy_constructor& other) = default;
  delete_copy_constructor& operator=(delete_copy_constructor&& other) = default;
  delete_copy_constructor& operator=(const delete_copy_constructor& other) = default;
};

template <typename T>
struct delete_copy_constructor<T, false> {
  delete_copy_constructor() = default;
  delete_copy_constructor(delete_copy_constructor&& other) = default;
  delete_copy_constructor(const delete_copy_constructor& other) = delete;
  delete_copy_constructor& operator=(delete_copy_constructor&& other) = default;
  delete_copy_constructor& operator=(const delete_copy_constructor& other) = default;
};

template <typename T, bool = std::is_move_assignable_v<T>>
struct delete_move_assignment {
  delete_move_assignment() = default;
  delete_move_assignment(delete_move_assignment&& other) = default;
  delete_move_assignment(const delete_move_assignment& other) = default;
  delete_move_assignment& operator=(delete_move_assignment&& other) = default;
  delete_move_assignment& operator=(const delete_move_assignment& other) = default;
};

template <typename T>
struct delete_move_assignment<T, false> {
  delete_move_assignment() = default;
  delete_move_assignment(delete_move_assignment&& other) = default;
  delete_move_assignment(const delete_move_assignment& other) = default;
  delete_move_assignment& operator=(delete_move_assignment&& other) = delete;
  delete_move_assignment& operator=(const delete_move_assignment& other) = default;
};

template <typename T, bool = std::is_copy_assignable_v<T>>
struct delete_copy_assignment {
  delete_copy_assignment() = default;
  delete_copy_assignment(delete_copy_assignment&& other) = default;
  delete_copy_assignment(const delete_copy_assignment& other) = default;
  delete_copy_assignment& operator=(delete_copy_assignment&& other) = default;
  delete_copy_assignment& operator=(const delete_copy_assignment& other) = default;
};

template <typename T>
struct delete_copy_assignment<T, false> {
  delete_copy_assignment() = default;
  delete_copy_assignment(delete_copy_assignment&& other) = default;
  delete_copy_assignment(const delete_copy_assignment& other) = default;
  delete_copy_assignment& operator=(delete_copy_assignment&& other) = default;
  delete_copy_assignment& operator=(const delete_copy_assignment& other) = delete;
};

template <typename Handler>
class scope_exit {
public:
  explicit scope_exit(Handler handler) noexcept : handler_(std::move(handler)) {}

  scope_exit(scope_exit&& other) noexcept :
    handler_(std::move(other.handler_)), invoke_(std::exchange(other.invoke_, false))
  {}

  scope_exit(const scope_exit& other) = delete;
  scope_exit& operator=(const scope_exit& other) = delete;

  ~scope_exit() noexcept(ICE_NO_EXCEPTIONS || noexcept(handler_()))
  {
    if (invoke_) {
      handler_();
    }
  }

private:
  Handler handler_;
  bool invoke_ = true;
};

template <typename Handler>
inline auto on_scope_exit(Handler&& handler) noexcept
{
  return scope_exit<Handler>{ std::forward<Handler>(handler) };
}

class thread_local_storage {
public:
  struct close_type {
    void operator()(std::uintptr_t handle) noexcept;
  };
#if ICE_OS_WIN32
  using handle_type = handle<std::uintptr_t, 0, close_type>;
#else
  using handle_type = handle<std::uintptr_t, -1, close_type>;
#endif
  using handle_view = handle_type::view;

  class lock {
  public:
    lock(handle_view handle, void* value) noexcept;

    lock(lock&& other) noexcept = default;
    lock& operator=(lock&& other) noexcept = default;

    ~lock();

  private:
    handle_view handle_;
  };

  thread_local_storage() noexcept;

  lock set(void* value) noexcept
  {
    return { handle_, value };
  }

  void* get() noexcept;
  const void* get() const noexcept;

private:
  handle_type handle_;
};

std::error_code set_thread_affinity(std::size_t index) noexcept;

// Copyright (c) 2010-2014, Sebastien Andrivet
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from this
//    software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

namespace xorstr_impl {

// clang-format off
constexpr auto time = __TIME__;
constexpr auto seed = static_cast<int>(time[7]) +
                      static_cast<int>(time[6]) * 10 +
                      static_cast<int>(time[4]) * 60 +
                      static_cast<int>(time[3]) * 600 +
                      static_cast<int>(time[1]) * 3600 +
                      static_cast<int>(time[0]) * 36000;
// clang-format on

// 1988, Stephen Park and Keith Miller
// "Random Number Generators: Good Ones Are Hard To Find", considered as "minimal standard"
// Park-Miller 31 bit pseudo-random number generator, implemented with G. Carta's optimisation:
// with 32-bit math and without division

template <int N>
struct random_generator {
private:
  // clang-format off
  static constexpr unsigned a   = 16807;                           // 7^5
  static constexpr unsigned m   = 2147483647;                      // 2^31 - 1
  static constexpr unsigned s   = random_generator<N - 1>::value;  // recursion
  static constexpr unsigned lo  = a * (s & 0xFFFF);                // multiply lower 16 bits by 16807
  static constexpr unsigned hi  = a * (s >> 16);                   // multiply higher 16 bits by 16807
  static constexpr unsigned lo2 = lo + ((hi & 0x7FFF) << 16);      // combine lower 15 bits of hi with lo's upper bits
  static constexpr unsigned hi2 = hi >> 15;                        // discard lower 15 bits of hi
  static constexpr unsigned lo3 = lo2 + hi;
  // clang-format on

public:
  static constexpr unsigned max = m;
  static constexpr unsigned value = lo3 > m ? lo3 - m : lo3;
};

template <>
struct random_generator<0> {
  static constexpr unsigned value = seed;
};

template <int N, int M>
struct random_int {
  static constexpr auto value = random_generator<N + 1>::value % M;
};

template <int N>
struct random_char {
  static const char value = static_cast<char>(1 + random_int<N, 0x7F - 1>::value);
};

template <size_t N, int K>
struct string {
private:
  const char key_;
  std::array<char, N + 1> encrypted_;

  constexpr char enc(char c) const
  {
    return c ^ key_;
  }

  char dec(char c) const
  {
    return c ^ key_;
  }

public:
  template <size_t... Is>
  constexpr ICE_ALWAYS_INLINE string(const char* str, std::index_sequence<Is...>) :
    key_(random_char<K>::value), encrypted_{ { enc(str[Is])... } }
  {}

  ICE_ALWAYS_INLINE decltype(auto) decrypt()
  {
    for (size_t i = 0; i < N; ++i) {
      encrypted_[i] = dec(encrypted_[i]);
    }
    encrypted_[N] = '\0';
    return encrypted_.data();
  }
};

}  // namespace xorstr_impl
}  // namespace ice

#define xorstr(s) \
  (ice::xorstr_impl::string<sizeof(s) - 1, __COUNTER__>(s, std::make_index_sequence<sizeof(s) - 1>()).decrypt())
