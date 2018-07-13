#pragma once
#include <ice/config.hpp>
#include <ice/error.hpp>
#include <ice/handle.hpp>
#include <ice/net/endpoint.hpp>
#include <ice/net/option.hpp>
#include <ice/service.hpp>
#include <functional>
#include <limits>
#include <cstdint>

namespace ice::net {

enum class shutdown {
  recv,
  send,
  both,
};

class socket {
public:
#if ICE_OS_WIN32
  struct close_type {
    void operator()(std::uintptr_t handle) noexcept;
  };
  using handle_type = ice::handle<std::uintptr_t, std::numeric_limits<std::uintptr_t>::max(), close_type>;
#else
  struct close_type {
    void operator()(int handle) noexcept;
  };
  using handle_type = ice::handle<int, -1, close_type>;
#endif

  explicit socket(ice::service& service) noexcept : service_(service) {}

  socket(socket&& other) noexcept = default;
  socket& operator=(socket&& other) noexcept = default;

  socket(const socket& other) = delete;
  socket& operator=(const socket& other) = delete;

  virtual ~socket() = default;

  constexpr explicit operator bool() const noexcept
  {
    return handle_.valid();
  }

  std::error_code create(int family, int type, int protocol = 0) noexcept;
  std::error_code bind(const endpoint& endpoint) noexcept;
  std::error_code shutdown(net::shutdown direction = net::shutdown::both) noexcept;

  void close() noexcept
  {
    handle_.reset();
  }

  constexpr int family() const noexcept
  {
    return family_;
  }

  int type() const noexcept;
  int protocol() const noexcept;
  net::endpoint name() const noexcept;

  std::error_code get(net::option& option) const noexcept
  {
    return get(option.level(), option.name(), option.data(), option.size());
  }

  std::error_code set(const net::option& option) noexcept
  {
    return set(option.level(), option.name(), option.data(), option.size());
  }

  std::error_code get(int level, int name, void* data, socklen_t& size) const noexcept;
  std::error_code set(int level, int name, const void* data, socklen_t size) noexcept;

  ice::service& service() const noexcept
  {
    return service_.get();
  }

  constexpr handle_type& handle() noexcept
  {
    return handle_;
  }

  constexpr const handle_type& handle() const noexcept
  {
    return handle_;
  }

protected:
  std::reference_wrapper<ice::service> service_;
  handle_type handle_;

private:
  int family_ = 0;
};

}  // namespace ice::net
