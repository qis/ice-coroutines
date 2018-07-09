#pragma once
#include <ice/config.hpp>
#include <ice/error.hpp>
#include <ice/net/buffer.hpp>
#include <ice/net/socket.hpp>
#include <ice/service.hpp>
#include <system_error>
#include <utility>
#include <cstddef>

namespace ice::net::tcp {

class accept;
class connect;
class recv;
class send;
class send_some;

class socket : public net::socket {
public:
  explicit socket(ice::service& service) noexcept : net::socket(service) {}

  socket(ice::service& service, int family);
  socket(ice::service& service, int family, int protocol);

  void create(int family);
  void create(int family, std::error_code& ec) noexcept;

  void create(int family, int protocol);
  void create(int family, int protocol, std::error_code& ec) noexcept;

  void listen();
  void listen(std::error_code& ec) noexcept;

  void listen(std::size_t backlog);
  void listen(std::size_t backlog, std::error_code& ec) noexcept;

  tcp::accept accept();
  tcp::accept accept(std::error_code& ec) noexcept;
  tcp::connect connect(const net::endpoint& endpoint);
  tcp::connect connect(const net::endpoint& endpoint, std::error_code& ec) noexcept;
  tcp::recv recv(char* data, std::size_t size);
  tcp::recv recv(char* data, std::size_t size, std::error_code& ec) noexcept;
  tcp::send send(const char* data, std::size_t size);
  tcp::send send(const char* data, std::size_t size, std::error_code& ec) noexcept;
  tcp::send_some send_some(const char* data, std::size_t size);
  tcp::send_some send_some(const char* data, std::size_t size, std::error_code& ec) noexcept;
};

class accept final : public native_event {
public:
  accept(socket& socket, std::error_code* ec) noexcept;

  bool await_ready() noexcept;
  bool await_suspend(std::experimental::coroutine_handle<> awaiter) noexcept;

  void resume() noexcept override;

  tcp::socket await_resume() noexcept(ICE_NO_EXCEPTIONS)
  {
    if (ec_) {
      throw_error(ec_, "accept tcp socket");
    }
    return std::move(client_);
  }

private:
  socket& socket_;
  socket client_;
  std::error_code* uec_ = nullptr;
};


inline tcp::accept socket::accept()
{
  return { *this, nullptr };
}

inline tcp::accept socket::accept(std::error_code& ec) noexcept
{
  return { *this, &ec };
}

#if 0
inline tcp::connect socket::connect(const net::endpoint& endpoint)
{
  return { *this, endpoint, nullptr };
}

inline tcp::connect socket::connect(const net::endpoint& endpoint, std::error_code& ec) noexcept
{
  return { *this, endpoint, &ec };
}

inline tcp::recv socket::recv(char* data, std::size_t size)
{
  return { *this, data, size, nullptr };
}

inline tcp::recv socket::recv(char* data, std::size_t size, std::error_code& ec) noexcept
{
  return { *this, data, size, &ec };
}

inline tcp::send socket::send(const char* data, std::size_t size)
{
  return { *this, data, size, nullptr };
}

inline tcp::send socket::send(const char* data, std::size_t size, std::error_code& ec) noexcept
{
  return { *this, data, size, &ec };
}

inline tcp::send_some socket::send_some(const char* data, std::size_t size)
{
  return { *this, data, size, nullptr };
}

inline tcp::send_some socket::send_some(const char* data, std::size_t size, std::error_code& ec) noexcept
{
  return { *this, data, size, &ec };
}
#endif

}  // namespace ice::net::tcp
