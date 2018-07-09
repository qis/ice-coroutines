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

  //tcp::accept accept();
  //tcp::connect connect(const net::endpoint& endpoint);
  //tcp::recv recv(char* data, std::size_t size);
  //tcp::send send(const char* data, std::size_t size);
  //tcp::send_some send_some(const char* data, std::size_t size);
};

}  // namespace ice::net::tcp
