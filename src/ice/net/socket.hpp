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
  using handle_view = handle_type::view;

  explicit socket(ice::service& service) noexcept : service_(service) {}

  socket(ice::service& service, int family, int type, int protocol = 0);

  socket(socket&& other) noexcept = default;
  socket& operator=(socket&& other) noexcept = default;

  socket(const socket& other) = delete;
  socket& operator=(const socket& other) = delete;

  virtual ~socket() = default;

  constexpr explicit operator bool() const noexcept
  {
    return handle_.valid();
  }

  void create(int family, int type);
  void create(int family, int type, std::error_code& ec) noexcept;

  void create(int family, int type, int protocol);
  void create(int family, int type, int protocol, std::error_code& ec) noexcept;

  void bind(const net::endpoint& endpoint);
  void bind(const net::endpoint& endpoint, std::error_code& ec) noexcept;

  void shutdown();
  void shutdown(std::error_code& ec) noexcept;

  void shutdown(net::shutdown direction);
  void shutdown(net::shutdown direction, std::error_code& ec) noexcept;

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

  template <typename T>
  std::enable_if_t<std::is_base_of_v<net::option, T>, T> get() const
  {
    std::error_code ec;
    const auto value = get<T>(ec);
    throw_on_error(ec, "get socket option");
    return value;
  }

  template <typename T>
  std::enable_if_t<std::is_base_of_v<net::option, T>, T> get(std::error_code& ec) const noexcept
  {
    T option;
    option.size() = option.capacity();
    get(option.level(), option.name(), option.data(), option.size(), ec);
    return option;
  }

  void set(const net::option& option)
  {
    std::error_code ec;
    set(option.level(), option.name(), option.data(), option.size(), ec);
    throw_on_error(ec, "set socket option");
  }

  void set(const net::option& option, std::error_code& ec) noexcept
  {
    set(option.level(), option.name(), option.data(), option.size(), ec);
  }

  void get(int level, int name, void* data, socklen_t& size, std::error_code& ec) const noexcept;
  void set(int level, int name, const void* data, socklen_t size, std::error_code& ec) noexcept;

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

  constexpr net::endpoint& remote_endpoint() noexcept
  {
    return remote_endpoint_;
  }

  constexpr const net::endpoint& remote_endpoint() const noexcept
  {
    return remote_endpoint_;
  }

  constexpr net::endpoint& local_endpoint() noexcept
  {
    return local_endpoint_;
  }

  constexpr const net::endpoint& local_endpoint() const noexcept
  {
    return local_endpoint_;
  }

protected:
  std::reference_wrapper<ice::service> service_;
  net::endpoint remote_endpoint_;
  net::endpoint local_endpoint_;
  handle_type handle_;

private:
  int family_ = 0;
};

}  // namespace ice::net
