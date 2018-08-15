#pragma once
#include <ice/async.hpp>
#include <ice/config.hpp>
#include <ice/handle.hpp>
#include <ice/net/tcp/socket.hpp>
#include <memory>
#include <system_error>
#include <cstdio>

#if ICE_OS_WIN32
#  include <array>
#endif

typedef struct _LIBSSH2_SESSION LIBSSH2_SESSION;
typedef struct _LIBSSH2_CHANNEL LIBSSH2_CHANNEL;

namespace ice::net::ssh {

class session {
public:
  struct session_destructor {
    void operator()(LIBSSH2_SESSION* handle) noexcept;
  };
  using session_handle = handle<LIBSSH2_SESSION*, nullptr, session_destructor>;

  struct channel_destructor {
    void operator()(LIBSSH2_CHANNEL* handle) noexcept;
  };
  using channel_handle = handle<LIBSSH2_CHANNEL*, nullptr, channel_destructor>;

  session(service& service) noexcept : socket_(service) {}

  session(session&& other) noexcept;
  session(const session& other) = delete;
  session& operator=(session&& other) noexcept;
  session& operator=(const session& other) = delete;

  explicit operator bool() const noexcept
  {
    return socket_ && session_ && channel_;
  }

  std::error_code create(int family) noexcept;

  async<std::error_code> connect(net::endpoint endpoint) noexcept;
  async<std::error_code> disconnect() noexcept;
  void close() noexcept;

  async<std::error_code> authenticate(std::string username, std::string password) noexcept;
  async<std::error_code> request_pty(std::string terminal) noexcept;
  async<std::error_code> open_shell() noexcept;

  async<int> exec(std::string command, std::error_code& ec) noexcept;

  async<std::size_t> recv(char* data, std::size_t size, std::error_code& ec) noexcept
  {
    return recv(stdout, data, size, ec);
  }

  async<std::size_t> recv(FILE* stream, char* data, std::size_t size, std::error_code& ec) noexcept;

  async<std::size_t> send(const char* data, std::size_t size, std::error_code& ec) noexcept
  {
    return send(stdout, data, size, ec);
  }

  async<std::size_t> send(FILE* stream, const char* data, std::size_t size, std::error_code& ec) noexcept;

  net::service& service() const noexcept
  {
    return socket_.service();
  }

private:
  enum class operation {
    none,
    recv,
    send,
  };

  async<std::error_code> io() noexcept;

  template <typename Function>
  async<std::error_code> loop(Function function) noexcept;

  friend long long on_recv(session& session, char* data, std::size_t size, int flags) noexcept;
  friend long long on_send(session& session, const char* data, std::size_t size, int flags) noexcept;

  tcp::socket socket_;
  session_handle session_;
  channel_handle channel_;
  //bool connected_ = false;

  operation operation_ = operation::none;
#if ICE_OS_WIN32
  std::array<char, 4096> storage_;
  std::size_t size_ = 0;
  bool ready_ = false;
#endif
};

}  // namespace ice::net::ssh
