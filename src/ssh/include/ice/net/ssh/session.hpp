#pragma once
#include <ice/async.hpp>
#include <ice/config.hpp>
#include <ice/handle.hpp>
#include <ice/net/ssh/channel.hpp>
#include <ice/net/ssh/types.hpp>
#include <ice/net/tcp/socket.hpp>
#include <memory>
#include <system_error>

#if ICE_OS_WIN32
#  include <array>
#endif

namespace ice::net::ssh {

class session {
public:
  enum class operation {
    none,
    recv,
    send,
  };

  struct close_type {
    void operator()(LIBSSH2_SESSION* handle) noexcept;
  };
  using handle_type = handle<LIBSSH2_SESSION*, nullptr, close_type>;
  using handle_view = handle_type::view;

  session(service& service) noexcept : socket_(service) {}

  session(session&& other) noexcept;
  session(const session& other) = delete;
  session& operator=(session&& other) noexcept;
  session& operator=(const session& other) = delete;

  ~session();

  std::error_code create(int family) noexcept;

  async<std::error_code> connect(net::endpoint endpoint) noexcept;
  async<void> disconnect() noexcept;

  async<std::error_code> authenticate(std::string username, std::string password) noexcept;
  async<channel> open() noexcept;

  async<std::error_code> io() noexcept;

  bool connected() const noexcept
  {
    return connected_;
  }

  net::service& service() const noexcept
  {
    return socket_.service();
  }

  handle_view handle() const noexcept
  {
    return handle_;
  }

private:
  friend long long on_recv(session& session, char* data, std::size_t size, int flags) noexcept;
  friend long long on_send(session& session, const char* data, std::size_t size, int flags) noexcept;

  tcp::socket socket_;
  handle_type handle_;
  bool connected_ = false;

  operation operation_ = operation::none;
#if ICE_OS_WIN32
  std::array<char, 4096> storage_;
  std::size_t size_ = 0;
  bool ready_ = false;
#endif
};

}  // namespace ice::net::ssh
