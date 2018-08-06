#pragma once
#include <ice/async.hpp>
#include <ice/config.hpp>
#include <ice/net/endpoint.hpp>
#include <ice/net/service.hpp>
#include <ice/net/socket.hpp>
#include <system_error>

namespace ice::net::tcp {

class socket : public net::socket {
public:
  explicit socket(net::service& service) noexcept : net::socket(service) {}

  std::error_code create(int family) noexcept;
  std::error_code create(int family, int protocol) noexcept;
  std::error_code listen(std::size_t backlog = 0) noexcept;

  async<socket> accept(endpoint& endpoint) noexcept;
  async<std::error_code> connect(const endpoint& endpoint) noexcept;
  async<std::size_t> recv(char* data, std::size_t size, std::error_code& ec) noexcept;
  async<std::size_t> send(const char* data, std::size_t size, std::error_code& ec) noexcept;
};

}  // namespace ice::net::tcp
