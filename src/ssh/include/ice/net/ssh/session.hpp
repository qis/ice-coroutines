#pragma once
#include <ice/async.hpp>
#include <ice/net/tcp/socket.hpp>
#include <system_error>

namespace ice::net::ssh {

class session {
public:
  session(service& service) : socket_(service) {}

  session(session&& other) = default;
  session(const session& other) = delete;
  session& operator=(session&& other) = default;
  session& operator=(const session& other) = delete;

  ~session() = default;

  async<std::error_code> connect(net::endpoint endpoint)
  {
    if (const auto ec = socket_.create(endpoint.family())) {
      co_return ec;
    }
    if (const auto ec = co_await socket_.connect(endpoint)) {
      co_return ec;
    }
    co_return {};
  }

  net::service& service() const
  {
    return socket_.service();
  }

  //ssh_session handle() noexcept
  //{
  //  return handle_.get();
  //}

  //const ssh_session handle() const noexcept
  //{
  //  return handle_.get();
  //}

private:
  net::tcp::socket socket_;
  //std::unique_ptr<ssh_session_struct, decltype(&ssh_free)> handle_ = { nullptr, ssh_free };
};

}  // namespace ice::net::ssh
