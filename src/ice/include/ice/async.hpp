// https://github.com/lewissbaker/cppcoro/compare/862e99d...master
//
// Copyright 2017 Lewis Baker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is furnished
// to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once
#include <ice/config.hpp>
#include <ice/error.hpp>
#include <ice/result.hpp>
#include <atomic>
#include <experimental/coroutine>
#include <iterator>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>
#include <cassert>
#include <cstddef>
#include <cstdio>

#if ICE_EXCEPTIONS
#  include <exception>
#  include <stdexcept>
#endif

namespace ice {

struct task {
  struct promise_type {
    constexpr task get_return_object() const noexcept
    {
      return {};
    }

    constexpr auto initial_suspend() const noexcept
    {
      return std::experimental::suspend_never{};
    }

    constexpr auto final_suspend() const noexcept
    {
      return std::experimental::suspend_never{};
    }

    constexpr void return_void() const noexcept {}

#if ICE_EXCEPTIONS || defined(__clang__)
    void unhandled_exception() const noexcept(ICE_NO_EXCEPTIONS)
    {
#  if ICE_EXCEPTIONS
      throw;
#  endif
    }
#endif
  };
};

// == include/cppcoro/generator.hpp ===================================================================================

template <typename T>
class generator;

namespace detail {

template <typename T>
class generator_promise {
public:
  using value_type = std::remove_reference_t<T>;
  using reference_type = std::conditional_t<std::is_reference_v<T>, T, T&>;
  using pointer_type = value_type*;

  generator_promise() = default;

  generator<T> get_return_object() noexcept;

  constexpr std::experimental::suspend_always initial_suspend() const
  {
    return {};
  }

  constexpr std::experimental::suspend_always final_suspend() const
  {
    return {};
  }

  template <typename U, typename = std::enable_if_t<std::is_same<U, T>::value>>
  std::experimental::suspend_always yield_value(U& value) noexcept
  {
    m_value = std::addressof(value);
    return {};
  }

  std::experimental::suspend_always yield_value(T&& value) noexcept
  {
    m_value = std::addressof(value);
    return {};
  }

#if ICE_EXCEPTIONS || defined(__clang__)
  void unhandled_exception()
  {
#  if ICE_EXCEPTIONS
    throw;
#  endif
  }
#endif

  void return_void() {}

  reference_type value() const noexcept
  {
    return *m_value;
  }

  template <typename U>
  std::experimental::suspend_never await_transform(U&& value) = delete;

private:
  pointer_type m_value;
};

template <typename T>
class generator_iterator {
  using coroutine_handle = std::experimental::coroutine_handle<generator_promise<T>>;

public:
  using iterator_category = std::input_iterator_tag;
  using difference_type = std::size_t;
  using value_type = std::remove_reference_t<T>;
  using reference = value_type&;
  using pointer = value_type*;

  explicit generator_iterator(std::nullptr_t) noexcept {}

  explicit generator_iterator(coroutine_handle coroutine) noexcept : m_coroutine(coroutine) {}

  bool operator==(const generator_iterator& other) const noexcept
  {
    return m_coroutine == other.m_coroutine;
  }

  bool operator!=(const generator_iterator& other) const noexcept
  {
    return !(*this == other);
  }

  generator_iterator& operator++()
  {
    m_coroutine.resume();
    if (m_coroutine.done()) {
      m_coroutine = nullptr;
    }
    return *this;
  }

  generator_iterator operator++(int) = delete;

  reference operator*() const noexcept
  {
    return m_coroutine.promise().value();
  }

  pointer operator->() const noexcept
  {
    return std::addressof(operator*());
  }

private:
  coroutine_handle m_coroutine = nullptr;
};

}  // namespace detail

template <typename T>
class generator {
public:
  using promise_type = detail::generator_promise<T>;
  using iterator = detail::generator_iterator<T>;

  generator() noexcept = default;

  generator(generator&& other) noexcept : m_coroutine(other.m_coroutine)
  {
    other.m_coroutine = nullptr;
  }

  generator(const generator& other) = delete;

  ~generator()
  {
    if (m_coroutine) {
      m_coroutine.destroy();
    }
  }

  generator& operator=(generator other) noexcept
  {
    swap(other);
    return *this;
  }

  iterator begin()
  {
    if (m_coroutine) {
      m_coroutine.resume();
      if (!m_coroutine.done()) {
        return iterator{ m_coroutine };
      }
    }
    return iterator{ nullptr };
  }

  iterator end() noexcept
  {
    return iterator{ nullptr };
  }

  void swap(generator& other) noexcept
  {
    std::swap(m_coroutine, other.m_coroutine);
  }

private:
  friend class detail::generator_promise<T>;

  explicit generator(std::experimental::coroutine_handle<promise_type> coroutine) noexcept : m_coroutine(coroutine) {}

  std::experimental::coroutine_handle<promise_type> m_coroutine = nullptr;
};

template <typename T>
void swap(generator<T>& a, generator<T>& b)
{
  a.swap(b);
}

namespace detail {

template <typename T>
generator<T> generator_promise<T>::get_return_object() noexcept
{
  using coroutine_handle = std::experimental::coroutine_handle<generator_promise<T>>;
  return generator<T>{ coroutine_handle::from_promise(*this) };
}

}  // namespace detail

// == include/cppcoro/broken_promise.hpp ==============================================================================

#if ICE_EXCEPTIONS

class broken_promise : public std::logic_error {
public:
  broken_promise() : std::logic_error("broken promise") {}
};

#endif

// == include/cppcoro/detail/continuation.hpp =========================================================================

namespace detail {

class continuation {
public:
  using callback_t = void(void*);

  continuation() noexcept = default;

  explicit continuation(std::experimental::coroutine_handle<> awaiter) noexcept :
    m_callback(nullptr), m_state(awaiter.address())
  {}

  explicit continuation(callback_t* callback, void* state) noexcept : m_callback(callback), m_state(state) {}

  explicit operator bool() const noexcept
  {
    return m_callback != nullptr || m_state != nullptr;
  }

  void resume() noexcept
  {
    if (m_callback == nullptr) {
      std::experimental::coroutine_handle<>::from_address(m_state).resume();
    } else {
      m_callback(m_state);
    }
  }

private:
  callback_t* m_callback = nullptr;
  void* m_state = nullptr;
};

}  // namespace detail

// == include/cppcoro/task.hpp ========================================================================================

template <typename T>
class async;

namespace detail {

class async_promise_base {
  friend struct final_awaitable;

  struct final_awaitable {
    bool await_ready() const noexcept
    {
      return false;
    }

    template <typename PROMISE>
    void await_suspend(std::experimental::coroutine_handle<PROMISE> coroutine)
    {
      async_promise_base& promise = coroutine.promise();
      if (promise.m_state.exchange(true, std::memory_order_acq_rel)) {
        promise.m_continuation.resume();
      }
    }

    void await_resume() noexcept {}
  };

public:
  async_promise_base() noexcept : m_state(false) {}

  auto initial_suspend() noexcept
  {
    return std::experimental::suspend_always{};
  }

  auto final_suspend() noexcept
  {
    return final_awaitable{};
  }

#if ICE_EXCEPTIONS || defined(__clang__)
  void unhandled_exception() noexcept
  {
#  if ICE_EXCEPTIONS
    m_exception = std::current_exception();
#  endif
  }
#endif

  bool try_set_continuation(continuation c)
  {
    m_continuation = c;
    return !m_state.exchange(true, std::memory_order_acq_rel);
  }

protected:
  bool completed() const noexcept
  {
    return m_state.load(std::memory_order_relaxed);
  }

  bool completed_with_unhandled_exception()
  {
#if ICE_EXCEPTIONS
    return m_exception != nullptr;
#else
    return false;
#endif
  }

  void rethrow_if_unhandled_exception()
  {
#if ICE_EXCEPTIONS
    if (m_exception != nullptr) {
      std::rethrow_exception(m_exception);
    }
#endif
  }

private:
  continuation m_continuation;
#if ICE_EXCEPTIONS
  std::exception_ptr m_exception;
#endif
  std::atomic<bool> m_state;
};

template <typename T>
class async_promise final : public async_promise_base {
public:
  async_promise() noexcept = default;

  ~async_promise()
  {
    if (completed() && !completed_with_unhandled_exception()) {
      reinterpret_cast<T*>(&m_valueStorage)->~T();
    }
  }

  async<T> get_return_object() noexcept;

  template <typename VALUE, typename = std::enable_if_t<std::is_convertible_v<VALUE&&, T>>>
  void return_value(VALUE&& value) noexcept(std::is_nothrow_constructible_v<T, VALUE&&>)
  {
    new (&m_valueStorage) T(std::forward<VALUE>(value));
  }

  void return_value(T value) noexcept(std::is_nothrow_move_constructible_v<T>)
  {
    new (&m_valueStorage) T(std::move(value));
  }

  T& result() &
  {
    rethrow_if_unhandled_exception();
    return *reinterpret_cast<T*>(&m_valueStorage);
  }

  T&& result() &&
  {
    rethrow_if_unhandled_exception();
    return std::move(*reinterpret_cast<T*>(&m_valueStorage));
  }

private:
#if _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4324)
#endif
  alignas(T) char m_valueStorage[sizeof(T)];
#if _MSC_VER
#  pragma warning(pop)
#endif
};

template <>
class async_promise<void> : public async_promise_base {
public:
  async_promise() noexcept = default;

  async<void> get_return_object() noexcept;

  void return_void() noexcept {}

  void result()
  {
    rethrow_if_unhandled_exception();
  }
};

template <typename T>
class async_promise<T&> : public async_promise_base {
public:
  async_promise() noexcept = default;

  async<T&> get_return_object() noexcept;

  void return_value(T& value) noexcept
  {
    m_value = std::addressof(value);
  }

  T& result()
  {
    rethrow_if_unhandled_exception();
    return *m_value;
  }

private:
  T* m_value = nullptr;
};

}  // namespace detail

template <typename T = void>
class async {
public:
  using promise_type = detail::async_promise<T>;
  using value_type = T;

private:
  struct awaitable_base {
    std::experimental::coroutine_handle<promise_type> m_coroutine;

    awaitable_base(std::experimental::coroutine_handle<promise_type> coroutine) noexcept : m_coroutine(coroutine) {}

    bool await_ready() const noexcept
    {
      return !m_coroutine || m_coroutine.done();
    }

    bool await_suspend(std::experimental::coroutine_handle<> awaiter) noexcept
    {
      m_coroutine.resume();
      return m_coroutine.promise().try_set_continuation(detail::continuation{ awaiter });
    }
  };

public:
  async() noexcept = default;

  explicit async(std::experimental::coroutine_handle<promise_type> coroutine) : m_coroutine(coroutine) {}

  async(async&& t) noexcept : m_coroutine(t.m_coroutine)
  {
    t.m_coroutine = nullptr;
  }

  async(const async&) = delete;
  async& operator=(const async&) = delete;

  ~async()
  {
    if (m_coroutine) {
      m_coroutine.destroy();
    }
  }

  async& operator=(async&& other) noexcept
  {
    if (std::addressof(other) != this) {
      if (m_coroutine) {
        m_coroutine.destroy();
      }
      m_coroutine = other.m_coroutine;
      other.m_coroutine = nullptr;
    }

    return *this;
  }

  bool is_ready() const noexcept
  {
    return !m_coroutine || m_coroutine.done();
  }

  auto operator co_await() const& noexcept
  {
    struct awaitable : awaitable_base {
      using awaitable_base::awaitable_base;

      decltype(auto) await_resume()
      {
#if ICE_EXCEPTIONS
        if (!this->m_coroutine) {
          throw broken_promise{};
        }
#else
        assert(this->m_coroutine);
#endif
        return this->m_coroutine.promise().result();
      }
    };
    return awaitable{ m_coroutine };
  }

  auto operator co_await() const&& noexcept
  {
    struct awaitable : awaitable_base {
      using awaitable_base::awaitable_base;

      decltype(auto) await_resume()
      {
#if ICE_EXCEPTIONS
        if (!this->m_coroutine) {
          throw broken_promise{};
        }
#else
        assert(this->m_coroutine);
#endif
        return std::move(this->m_coroutine.promise()).result();
      }
    };

    return awaitable{ m_coroutine };
  }

  auto when_ready() const noexcept
  {
    struct awaitable : awaitable_base {
      using awaitable_base::awaitable_base;

      void await_resume() const noexcept {}
    };

    return awaitable{ m_coroutine };
  }

  auto get_starter() const noexcept
  {
    class starter {
    public:
      starter(std::experimental::coroutine_handle<promise_type> coroutine) noexcept : m_coroutine(coroutine) {}

      void start(detail::continuation continuation) noexcept
      {
        if (m_coroutine && !m_coroutine.done()) {
          m_coroutine.resume();
          if (m_coroutine.promise().try_set_continuation(continuation)) {
            return;
          }
        }
        continuation.resume();
      }

    private:
      std::experimental::coroutine_handle<promise_type> m_coroutine;
    };

    return starter{ m_coroutine };
  }

private:
  std::experimental::coroutine_handle<promise_type> m_coroutine = nullptr;
};

namespace detail {

template <typename T>
async<T> async_promise<T>::get_return_object() noexcept
{
  return async<T>{ std::experimental::coroutine_handle<async_promise>::from_promise(*this) };
}

inline async<void> async_promise<void>::get_return_object() noexcept
{
  return async<void>{ std::experimental::coroutine_handle<async_promise>::from_promise(*this) };
}

template <typename T>
async<T&> async_promise<T&>::get_return_object() noexcept
{
  return async<T&>{ std::experimental::coroutine_handle<async_promise>::from_promise(*this) };
}

}  // namespace detail

// == include/cppcoro/async_generator.hpp =============================================================================
template <typename T>
class async_generator;

namespace detail {

template <typename T>
class async_generator_iterator;
class async_generator_yield_operation;
class async_generator_advance_operation;

class async_generator_promise_base {
public:
  async_generator_promise_base() noexcept = default;

  async_generator_promise_base(const async_generator_promise_base& other) = delete;
  async_generator_promise_base& operator=(const async_generator_promise_base& other) = delete;

  std::experimental::suspend_always initial_suspend() const noexcept
  {
    return {};
  }

  async_generator_yield_operation final_suspend() noexcept;

#if ICE_EXCEPTIONS || defined(__clang__)
  void unhandled_exception() noexcept
  {
#  if ICE_EXCEPTIONS
    if (m_state.load(std::memory_order_relaxed) != state::cancelled) {
      m_exception = std::current_exception();
    }
#  endif
  }
#endif

  void return_void() noexcept {}

  bool finished() const noexcept
  {
    return m_currentValue == nullptr;
  }

  void rethrow_if_unhandled_exception()
  {
#if ICE_EXCEPTIONS
    if (m_exception) {
      std::rethrow_exception(std::move(m_exception));
    }
#endif
  }

  bool request_cancellation() noexcept
  {
    const auto previousState = m_state.exchange(state::cancelled, std::memory_order_acq_rel);
    assert(previousState != state::value_not_ready_consumer_suspended);
    assert(previousState != state::cancelled);
    return previousState == state::value_ready_producer_suspended;
  }

protected:
  async_generator_yield_operation internal_yield_value() noexcept;

private:
  friend class async_generator_yield_operation;
  friend class async_generator_advance_operation;

  enum class state {
    value_not_ready_consumer_active,
    value_not_ready_consumer_suspended,
    value_ready_producer_active,
    value_ready_producer_suspended,
    cancelled,
  };

  std::atomic<state> m_state = state::value_ready_producer_suspended;
  std::experimental::coroutine_handle<> m_consumerCoroutine;
#if ICE_EXCEPTIONS
  std::exception_ptr m_exception = nullptr;
#endif

protected:
  void* m_currentValue;
};

class async_generator_yield_operation final {
  using state = async_generator_promise_base::state;

public:
  async_generator_yield_operation(async_generator_promise_base& promise, state initialState) noexcept :
    m_promise(promise), m_initialState(initialState)
  {}

  bool await_ready() const noexcept
  {
    return m_initialState == state::value_not_ready_consumer_suspended;
  }

  bool await_suspend(std::experimental::coroutine_handle<> producer) noexcept;

  void await_resume() noexcept {}

private:
  async_generator_promise_base& m_promise;
  state m_initialState;
};

inline async_generator_yield_operation async_generator_promise_base::final_suspend() noexcept
{
  m_currentValue = nullptr;
  return internal_yield_value();
}

inline async_generator_yield_operation async_generator_promise_base::internal_yield_value() noexcept
{
  state currentState = m_state.load(std::memory_order_acquire);
  assert(currentState != state::value_ready_producer_active);
  assert(currentState != state::value_ready_producer_suspended);
  if (currentState == state::value_not_ready_consumer_suspended) {
    m_state.store(state::value_ready_producer_active, std::memory_order_relaxed);
    m_consumerCoroutine.resume();
    currentState = m_state.load(std::memory_order_acquire);
  }
  return async_generator_yield_operation{ *this, currentState };
}

inline bool async_generator_yield_operation::await_suspend(std::experimental::coroutine_handle<> producer) noexcept
{
  state currentState = m_initialState;
  if (currentState == state::value_not_ready_consumer_active) {
    bool producerSuspended = m_promise.m_state.compare_exchange_strong(
      currentState, state::value_ready_producer_suspended, std::memory_order_release, std::memory_order_acquire);
    if (producerSuspended) {
      return true;
    }
    if (currentState == state::value_not_ready_consumer_suspended) {
      m_promise.m_state.store(state::value_ready_producer_active, std::memory_order_relaxed);
      m_promise.m_consumerCoroutine.resume();
      currentState = m_promise.m_state.load(std::memory_order_acquire);
      if (currentState == state::value_not_ready_consumer_suspended) {
        return false;
      }
    }
  }
  if (currentState == state::value_ready_producer_active) {
    const bool suspendedProducer = m_promise.m_state.compare_exchange_strong(
      currentState, state::value_ready_producer_suspended, std::memory_order_release, std::memory_order_acquire);
    if (suspendedProducer) {
      return true;
    }
    if (currentState == state::value_not_ready_consumer_suspended) {
      return false;
    }
  }
  assert(currentState == state::cancelled);
  producer.destroy();
  return true;
}

class async_generator_advance_operation {
  using state = async_generator_promise_base::state;

protected:
  async_generator_advance_operation(std::nullptr_t) noexcept {}

  async_generator_advance_operation(
    async_generator_promise_base& promise,
    std::experimental::coroutine_handle<> producerCoroutine) noexcept :
    m_promise(std::addressof(promise)),
    m_producerCoroutine(producerCoroutine)
  {
    state initialState = promise.m_state.load(std::memory_order_acquire);
    if (initialState == state::value_ready_producer_suspended) {
      promise.m_state.store(state::value_not_ready_consumer_active, std::memory_order_relaxed);
      producerCoroutine.resume();
      initialState = promise.m_state.load(std::memory_order_acquire);
    }
    m_initialState = initialState;
  }

public:
  bool await_ready() const noexcept
  {
    return m_initialState == state::value_ready_producer_suspended;
  }

  bool await_suspend(std::experimental::coroutine_handle<> consumerCoroutine) noexcept
  {
    m_promise->m_consumerCoroutine = consumerCoroutine;
    auto currentState = m_initialState;
    if (currentState == state::value_ready_producer_active) {
      if (m_promise->m_state.compare_exchange_strong(
            currentState, state::value_not_ready_consumer_suspended, std::memory_order_release,
            std::memory_order_acquire)) {
        return true;
      }
      assert(currentState == state::value_ready_producer_suspended);
      m_promise->m_state.store(state::value_not_ready_consumer_active, std::memory_order_relaxed);
      m_producerCoroutine.resume();
      currentState = m_promise->m_state.load(std::memory_order_acquire);
      if (currentState == state::value_ready_producer_suspended) {
        return false;
      }
    }
    assert(currentState == state::value_not_ready_consumer_active);
    return m_promise->m_state.compare_exchange_strong(
      currentState, state::value_not_ready_consumer_suspended, std::memory_order_release, std::memory_order_acquire);
  }

protected:
  async_generator_promise_base* m_promise = nullptr;
  std::experimental::coroutine_handle<> m_producerCoroutine = nullptr;

private:
  state m_initialState;
};

template <typename T>
class async_generator_promise final : public async_generator_promise_base {
  using value_type = std::remove_reference_t<T>;

public:
  async_generator_promise() noexcept = default;

  async_generator<T> get_return_object() noexcept;

  async_generator_yield_operation yield_value(value_type& value) noexcept
  {
    m_currentValue = std::addressof(value);
    return internal_yield_value();
  }

  async_generator_yield_operation yield_value(value_type&& value) noexcept
  {
    return yield_value(value);
  }

  T& value() const noexcept
  {
    return *static_cast<T*>(m_currentValue);
  }
};

template <typename T>
class async_generator_promise<T&&> final : public async_generator_promise_base {
public:
  async_generator_promise() noexcept = default;

  async_generator<T> get_return_object() noexcept;

  async_generator_yield_operation yield_value(T&& value) noexcept
  {
    m_currentValue = std::addressof(value);
    return internal_yield_value();
  }

  T&& value() const noexcept
  {
    return std::move(*static_cast<T*>(m_currentValue));
  }
};

template <typename T>
class async_generator_increment_operation final : public async_generator_advance_operation {
public:
  async_generator_increment_operation(async_generator_iterator<T>& iterator) noexcept :
    async_generator_advance_operation(iterator.m_coroutine.promise(), iterator.m_coroutine), m_iterator(iterator)
  {}

  async_generator_iterator<T>& await_resume();

private:
  async_generator_iterator<T>& m_iterator;
};

template <typename T>
class async_generator_iterator final {
  using promise_type = async_generator_promise<T>;
  using handle_type = std::experimental::coroutine_handle<promise_type>;

public:
  using iterator_category = std::input_iterator_tag;
  using difference_type = std::size_t;
  using value_type = std::remove_reference_t<T>;
  using reference = std::add_lvalue_reference_t<T>;
  using pointer = std::add_pointer_t<value_type>;

  async_generator_iterator(std::nullptr_t) noexcept {}

  async_generator_iterator(handle_type coroutine) noexcept : m_coroutine(coroutine) {}

  async_generator_increment_operation<T> operator++() noexcept
  {
    return async_generator_increment_operation<T>{ *this };
  }

  reference operator*() const noexcept
  {
    return m_coroutine.promise().value();
  }

  bool operator==(const async_generator_iterator& other) const noexcept
  {
    return m_coroutine == other.m_coroutine;
  }

  bool operator!=(const async_generator_iterator& other) const noexcept
  {
    return !(*this == other);
  }

private:
  friend class async_generator_increment_operation<T>;

  handle_type m_coroutine = nullptr;
};

template <typename T>
async_generator_iterator<T>& async_generator_increment_operation<T>::await_resume()
{
  if (m_promise->finished()) {
    m_iterator = async_generator_iterator<T>{ nullptr };
    m_promise->rethrow_if_unhandled_exception();
  }
  return m_iterator;
}

template <typename T>
class async_generator_begin_operation final : public async_generator_advance_operation {
  using promise_type = async_generator_promise<T>;
  using handle_type = std::experimental::coroutine_handle<promise_type>;

public:
  async_generator_begin_operation(std::nullptr_t) noexcept : async_generator_advance_operation(nullptr) {}

  async_generator_begin_operation(handle_type producerCoroutine) noexcept :
    async_generator_advance_operation(producerCoroutine.promise(), producerCoroutine)
  {}

  bool await_ready() const noexcept
  {
    return m_promise == nullptr || async_generator_advance_operation::await_ready();
  }

  async_generator_iterator<T> await_resume()
  {
    if (m_promise == nullptr) {
      return async_generator_iterator<T>{ nullptr };
    } else if (m_promise->finished()) {
      m_promise->rethrow_if_unhandled_exception();
      return async_generator_iterator<T>{ nullptr };
    }
    return async_generator_iterator<T>{ handle_type::from_promise(*static_cast<promise_type*>(m_promise)) };
  }
};

}  // namespace detail

template <typename T>
class async_generator {
public:
  using promise_type = detail::async_generator_promise<T>;
  using iterator = detail::async_generator_iterator<T>;

  async_generator() noexcept = default;

  explicit async_generator(promise_type& promise) noexcept :
    m_coroutine(std::experimental::coroutine_handle<promise_type>::from_promise(promise))
  {}

  async_generator(async_generator&& other) noexcept : m_coroutine(other.m_coroutine)
  {
    other.m_coroutine = nullptr;
  }

  ~async_generator()
  {
    if (m_coroutine) {
      if (m_coroutine.promise().request_cancellation()) {
        m_coroutine.destroy();
      }
    }
  }

  async_generator& operator=(async_generator&& other) noexcept
  {
    async_generator temp(std::move(other));
    swap(temp);
    return *this;
  }

  async_generator(const async_generator&) = delete;
  async_generator& operator=(const async_generator&) = delete;

  auto begin() noexcept
  {
    if (!m_coroutine) {
      return detail::async_generator_begin_operation<T>{ nullptr };
    }
    return detail::async_generator_begin_operation<T>{ m_coroutine };
  }

  auto end() noexcept
  {
    return iterator{ nullptr };
  }

  void swap(async_generator& other) noexcept
  {
    using std::swap;
    swap(m_coroutine, other.m_coroutine);
  }

private:
  std::experimental::coroutine_handle<promise_type> m_coroutine = nullptr;
};

template <typename T>
void swap(async_generator<T>& a, async_generator<T>& b) noexcept
{
  a.swap(b);
}

namespace detail {

template <typename T>
async_generator<T> async_generator_promise<T>::get_return_object() noexcept
{
  return async_generator<T>{ *this };
}

}  // namespace detail

// == include/cppcoro/async_mutex.hpp =================================================================================

class async_mutex_lock;
class async_mutex_lock_operation;
class async_mutex_scoped_lock_operation;

class async_mutex {
public:
  async_mutex() noexcept = default;
  ~async_mutex();

  bool try_lock() noexcept;
  async_mutex_lock_operation lock_async() noexcept;
  async_mutex_scoped_lock_operation scoped_lock_async() noexcept;
  void unlock();

private:
  friend class async_mutex_lock_operation;

  static constexpr std::uintptr_t not_locked = 1;
  static constexpr std::uintptr_t locked_no_waiters = 0;
  std::atomic<std::uintptr_t> m_state = not_locked;
  async_mutex_lock_operation* m_waiters = nullptr;
};

class async_mutex_lock {
public:
  explicit async_mutex_lock(async_mutex& mutex, std::adopt_lock_t) noexcept : m_mutex(&mutex) {}

  async_mutex_lock(async_mutex_lock&& other) noexcept : m_mutex(other.m_mutex)
  {
    other.m_mutex = nullptr;
  }

  async_mutex_lock(const async_mutex_lock& other) = delete;
  async_mutex_lock& operator=(const async_mutex_lock& other) = delete;

  ~async_mutex_lock()
  {
    if (m_mutex != nullptr) {
      m_mutex->unlock();
    }
  }

private:
  async_mutex* m_mutex;
};

class async_mutex_lock_operation {
public:
  explicit async_mutex_lock_operation(async_mutex& mutex) noexcept : m_mutex(mutex) {}

  bool await_ready() const noexcept
  {
    return false;
  }

  bool await_suspend(std::experimental::coroutine_handle<> awaiter) noexcept;

  void await_resume() const noexcept {}

protected:
  friend class async_mutex;

  async_mutex& m_mutex;

private:
  async_mutex_lock_operation* m_next;
  std::experimental::coroutine_handle<> m_awaiter;
};

class async_mutex_scoped_lock_operation : public async_mutex_lock_operation {
public:
  using async_mutex_lock_operation::async_mutex_lock_operation;

  [[nodiscard]] async_mutex_lock await_resume() const noexcept
  {
    return async_mutex_lock{ m_mutex, std::adopt_lock };
  }
};

// == lib/async_mutex.cpp =============================================================================================

inline async_mutex::~async_mutex()
{
  [[maybe_unused]] auto state = m_state.load(std::memory_order_relaxed);
  assert(state == not_locked || state == locked_no_waiters);
  assert(m_waiters == nullptr);
}

inline bool async_mutex::try_lock() noexcept
{
  auto oldState = not_locked;
  return m_state.compare_exchange_strong(
    oldState, locked_no_waiters, std::memory_order_acquire, std::memory_order_relaxed);
}

inline async_mutex_lock_operation async_mutex::lock_async() noexcept
{
  return async_mutex_lock_operation{ *this };
}

inline async_mutex_scoped_lock_operation async_mutex::scoped_lock_async() noexcept
{
  return async_mutex_scoped_lock_operation{ *this };
}

inline void async_mutex::unlock()
{
  assert(m_state.load(std::memory_order_relaxed) != not_locked);
  async_mutex_lock_operation* waitersHead = m_waiters;
  if (waitersHead == nullptr) {
    auto oldState = locked_no_waiters;
    const bool releasedLock =
      m_state.compare_exchange_strong(oldState, not_locked, std::memory_order_release, std::memory_order_relaxed);
    if (releasedLock) {
      return;
    }
    oldState = m_state.exchange(locked_no_waiters, std::memory_order_acquire);
    assert(oldState != locked_no_waiters && oldState != not_locked);
    auto* next = reinterpret_cast<async_mutex_lock_operation*>(oldState);
    do {
      auto* temp = next->m_next;
      next->m_next = waitersHead;
      waitersHead = next;
      next = temp;
    } while (next != nullptr);
  }
  assert(waitersHead != nullptr);
  m_waiters = waitersHead->m_next;
  waitersHead->m_awaiter.resume();
}

inline bool async_mutex_lock_operation::await_suspend(std::experimental::coroutine_handle<> awaiter) noexcept
{
  m_awaiter = awaiter;
  std::uintptr_t oldState = m_mutex.m_state.load(std::memory_order_acquire);
  while (true) {
    if (oldState == async_mutex::not_locked) {
      if (m_mutex.m_state.compare_exchange_weak(
            oldState, async_mutex::locked_no_waiters, std::memory_order_acquire, std::memory_order_relaxed)) {
        return false;
      }
    } else {
      m_next = reinterpret_cast<async_mutex_lock_operation*>(oldState);
      if (m_mutex.m_state.compare_exchange_weak(
            oldState, reinterpret_cast<std::uintptr_t>(this), std::memory_order_release, std::memory_order_relaxed)) {
        return true;
      }
    }
  }
}

template <typename T>
using async_result = async<result<T>>;

// clang-format off

template <typename Stream, typename Buffer>
inline async<std::size_t> recv(Stream& stream, Buffer& buffer, std::error_code& ec)
  noexcept(noexcept(stream.recv(std::data(buffer), std::size(buffer), ec)))
{
  return stream.recv(std::data(buffer), std::size(buffer), ec);
}

template <typename Stream, typename Buffer>
inline async<std::size_t> send(Stream& stream, const Buffer& buffer, std::error_code& ec)
  noexcept(noexcept(stream.send(std::data(buffer), std::size(buffer), ec)))
{
  return stream.send(std::data(buffer), std::size(buffer), ec);
}

template <typename Stream, typename Buffer>
inline async<std::error_code> send(Stream& stream, const Buffer& buffer)
  noexcept(noexcept(stream.send(std::data(buffer), std::size(buffer), std::declval<std::error_code&>())))
{
  std::error_code ec;
  const auto data = std::data(buffer);
  const auto size = std::size(buffer);
  const auto sent = co_await stream.send(data, size, ec);
  if (!ec && sent < size) {
    co_return make_error_code(errc::eof);
  }
  co_return ec;
}

// clang-format on

}  // namespace ice
