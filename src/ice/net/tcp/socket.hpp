#pragma once
#include <ice/async.hpp>
#include <ice/config.hpp>
#include <ice/net/endpoint.hpp>
#include <ice/net/socket.hpp>
#include <ice/service.hpp>
#include <system_error>

namespace ice::net::tcp {

class socket : public net::socket {
public:
  explicit socket(ice::service& service) noexcept : net::socket(service) {}

  std::error_code create(int family) noexcept;
  std::error_code create(int family, int protocol) noexcept;
  std::error_code listen(std::size_t backlog = 0) noexcept;

  async<tcp::socket> accept(net::endpoint& endpoint) noexcept;
  async<std::error_code> connect(const net::endpoint& endpoint) noexcept;
  async<std::size_t> recv(char* data, std::size_t size) noexcept;
  async<std::size_t> send(const char* data, std::size_t size) noexcept;
};

}  // namespace ice::net::tcp
