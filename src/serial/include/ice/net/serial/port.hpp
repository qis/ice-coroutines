#pragma once
#include <ice/async.hpp>
#include <ice/handle.hpp>
#include <ice/net/service.hpp>
#include <ice/net/tcp/socket.hpp>
#include <functional>
#include <system_error>

namespace ice::net::serial {

class port {
public:
#if ICE_OS_WIN32
  struct close_type {
    void operator()(std::uintptr_t handle) noexcept;
  };
  using handle_type = handle<std::uintptr_t, 0, close_type>;
#else
  struct close_type {
    void operator()(int handle) noexcept;
  };
  using handle_type = handle<int, -1, close_type>;
#endif
  using handle_view = handle_type::view;

  port(ice::net::service& service) : service_(service) {}

  std::error_code open(std::string device) noexcept;

  void close() noexcept
  {
    handle_.reset();
  }

  async<std::size_t> recv(char* data, std::size_t size, std::error_code& ec) noexcept;
  async<std::size_t> send(const char* data, std::size_t size, std::error_code& ec) noexcept;

  service& service() const noexcept
  {
    return service_.get();
  }

  handle_view handle() const noexcept
  {
    return handle_;
  }

private:
  std::reference_wrapper<net::service> service_;
  handle_type handle_;
};

}  // namespace ice::net::serial
