#include "ice/net/ssh/session.hpp"
#include <ice/net/event.hpp>
#include <ice/net/ssh/error.hpp>
#include <libssh2.h>
#include <cstdlib>

#if ICE_OS_WIN32
#  include <windows.h>
#  include <winsock2.h>
#else
#  include <sys/socket.h>
#  include <sys/types.h>
#endif

namespace ice::net::ssh {
namespace {

static LIBSSH2_RECV_FUNC(recv_callback)
{
  (void)socket;
  const auto data = reinterpret_cast<char*>(buffer);
  const auto size = static_cast<std::size_t>(length);
  if (!*abstract) {
    return ::recv(socket, data, socklen_t(size), 0);
  }
  return on_recv(*reinterpret_cast<session*>(*abstract), data, size, flags);
}

static LIBSSH2_SEND_FUNC(send_callback)
{
  (void)socket;
  const auto data = reinterpret_cast<const char*>(buffer);
  const auto size = static_cast<std::size_t>(length);
  if (!*abstract) {
    return ::send(socket, data, socklen_t(size), 0);
  }
  return on_send(*reinterpret_cast<session*>(*abstract), data, size, flags);
}

}  // namespace

void session::session_destructor::operator()(LIBSSH2_SESSION* handle) noexcept
{
  libssh2_session_set_blocking(handle, 1);
  libssh2_session_free(handle);
}

void session::channel_destructor::operator()(LIBSSH2_CHANNEL* handle) noexcept
{
  libssh2_channel_close(handle);
}

session::session(session&& other) noexcept :
  socket_(std::move(other.socket_)), session_(std::move(other.session_)), channel_(std::move(other.channel_))
{
  operation_ = other.operation_;
#if ICE_OS_WIN32
  storage_ = other.storage_;
  size_ = other.size_;
  ready_ = other.ready_;
#endif
  *libssh2_session_abstract(session_) = this;
}

session& session::operator=(session&& other) noexcept
{
  socket_ = std::move(other.socket_);
  session_ = std::move(other.session_);
  channel_ = std::move(other.channel_);
  operation_ = other.operation_;
#if ICE_OS_WIN32
  storage_ = other.storage_;
  size_ = other.size_;
  ready_ = other.ready_;
#endif
  *libssh2_session_abstract(session_) = this;
  return *this;
}

async<std::error_code> session::connect(net::endpoint endpoint) noexcept
{
  struct library {
    library()
    {
      if (const auto rc = libssh2_init(0)) {
        ec = make_error_code(rc, domain_category());
      }
    }
    ~library()
    {
      libssh2_exit();
    }
    std::error_code ec;
  };
  static const library library;
  if (library.ec) {
    co_return library.ec;
  }
  close();
  tcp::socket socket{ service() };
  if (const auto ec = socket.create(endpoint.family())) {
    co_return ec;
  }
  session_handle session{ libssh2_session_init_ex(nullptr, nullptr, nullptr, this) };
  if (!session) {
    co_return make_error_code(LIBSSH2_ERROR_ALLOC, domain_category());
  }
  if (const auto rc = libssh2_session_flag(session, LIBSSH2_FLAG_COMPRESS, 1)) {
    co_return make_error_code(rc, domain_category());
  }
  libssh2_session_callback_set(session, LIBSSH2_CALLBACK_RECV, reinterpret_cast<void*>(&recv_callback));
  libssh2_session_callback_set(session, LIBSSH2_CALLBACK_SEND, reinterpret_cast<void*>(&send_callback));
  libssh2_session_set_blocking(session, 0);
  if (const auto ec = co_await socket.connect(endpoint)) {
    co_return ec;
  }
  session_ = std::move(session);
  socket_ = std::move(socket);
  if (const auto ec = co_await loop([&]() { return libssh2_session_handshake(session_, socket_.handle()); })) {
    session_.reset();
    socket_.close();
    co_return ec;
  }
  co_return{};
}

async<std::error_code> session::disconnect() noexcept
{
  if (channel_) {
    if (const auto ec = co_await loop([&]() { return libssh2_channel_close(channel_); })) {
      co_return ec;
    }
    channel_.reset();
  }
  if (session_) {
    if (const auto ec = co_await loop([&]() { return libssh2_session_disconnect(session_, "shutdown"); })) {
      co_return ec;
    }
    session_.reset();
  }
  if (socket_) {
    socket_.close();
  }
}

void session::close() noexcept
{
  if (channel_) {
    channel_.reset();
  }
  if (session_) {
    session_.reset();
  }
  if (socket_) {
    socket_.close();
  }
}

async<std::error_code> session::authenticate(std::string username, std::string password) noexcept
{
  auto ec = co_await loop([this, username = std::move(username), password = std::move(password)]() {
    const auto udata = username.data();
    const auto usize = static_cast<unsigned int>(username.size());
    const auto pdata = password.data();
    const auto psize = static_cast<unsigned int>(password.size());
    return libssh2_userauth_password_ex(session_, udata, usize, pdata, psize, nullptr);
  });
  while (!ec) {
    channel_.reset(libssh2_channel_open_session(session_));
    if (channel_) {
      break;
    }
    if (const auto rc = libssh2_session_last_error(session_, nullptr, nullptr, 0); rc != LIBSSH2_ERROR_EAGAIN) {
      ec = make_error_code(rc, domain_category());
      break;
    }
    co_await io();
  }
  co_return ec;
}

async<std::error_code> session::request_pty(std::string terminal) noexcept
{
  return loop([&, terminal = std::move(terminal)]() { return libssh2_channel_request_pty(channel_, terminal.data()); });
}

async<std::error_code> session::open_shell() noexcept
{
  return loop([&]() { return libssh2_channel_shell(channel_); });
}

async<int> session::exec(std::string command, std::error_code& ec) noexcept
{
  ec = co_await loop([&, command = std::move(command)]() { return libssh2_channel_exec(channel_, command.data()); });
  co_return ec ? EXIT_FAILURE : libssh2_channel_get_exit_status(channel_);
}

async<std::size_t> session::recv(FILE* stream, char* data, std::size_t size, std::error_code& ec) noexcept
{
  const auto stream_id = stream == stderr ? SSH_EXTENDED_DATA_STDERR : 0;
  while (true) {
    if (const auto rc = libssh2_channel_read_ex(channel_, stream_id, data, size); rc >= 0) {
      co_return static_cast<std::size_t>(rc);
    } else if (rc != LIBSSH2_ERROR_EAGAIN) {
      if (rc != LIBSSH2_ERROR_NONE) {
        ec = make_error_code(rc);
      }
      break;
    }
    co_await io();
  }
  co_return{};
}

async<std::size_t> session::send(FILE* stream, const char* data, std::size_t size, std::error_code& ec) noexcept
{
  const auto stream_id = stream == stderr ? SSH_EXTENDED_DATA_STDERR : 0;
  const auto data_size = size;
  do {
    if (const auto rc = libssh2_channel_write_ex(channel_, stream_id, data, size); rc > 0) {
      data += static_cast<std::size_t>(rc);
      size -= static_cast<std::size_t>(rc);
      continue;
    } else if (rc != LIBSSH2_ERROR_EAGAIN) {
      if (rc != LIBSSH2_ERROR_NONE) {
        ec = make_error_code(rc);
      }
      break;
    }
    co_await io();
  } while (size > 0);
  co_return data_size - size;
}

async<std::error_code> session::io() noexcept
{
  std::error_code ec;
#if ICE_OS_WIN32
  switch (operation_) {
  case operation::recv: size_ = co_await socket_.recv(storage_.data(), size_, ec); break;
  case operation::send: size_ = co_await socket_.send(storage_.data(), size_, ec); break;
  default: ec = make_error_code(std::errc::invalid_argument); break;
  }
  if (!size_ && !ec) {
    ec = make_error_code(errc::eof);
  }
  ready_ = true;
#else
  switch (operation_) {
  case operation::recv: ec = co_await event{ service(), socket_.handle(), ICE_EVENT_RECV }; break;
  case operation::send: ec = co_await event{ service(), socket_.handle(), ICE_EVENT_RECV }; break;
  default: ec = make_error_code(std::errc::invalid_argument); break;
  }
#endif
  co_return ec;
}

template <typename Function>
inline async<std::error_code> session::loop(Function function) noexcept
{
  while (true) {
    const auto rc = function();
    if (rc == LIBSSH2_ERROR_NONE) {
      break;
    }
    if (rc != LIBSSH2_ERROR_EAGAIN) {
      co_return make_error_code(rc, domain_category());
    }
    co_await io();
  }
  co_return{};
}

long long on_recv(session& session, char* data, std::size_t size, int flags) noexcept
{
#if ICE_OS_WIN32
  (void)flags;
  if (std::exchange(session.ready_, false)) {
    assert(session.size_ <= size);
    std::memcpy(data, session.storage_.data(), session.size_);
    return static_cast<ssize_t>(session.size_);
  }
  session.operation_ = session::operation::recv;
  session.size_ = std::min(size, session.storage_.size());
  return EAGAIN > 0 ? -EAGAIN : EAGAIN;
#else
  do {
    if (const auto rc = ::recv(session.socket_.handle(), data, size, flags); rc >= 0) {
      return rc;
    }
  } while (errno == EINTR);
  session.operation_ = session::operation::recv;
  return errno > 0 ? -errno : errno;
#endif
}

long long on_send(session& session, const char* data, std::size_t size, int flags) noexcept
{
#if ICE_OS_WIN32
  (void)flags;
  if (std::exchange(session.ready_, false)) {
    assert(session.size_ <= size);
    if (std::memcmp(data, session.storage_.data(), session.size_) != 0) {
      assert(false);
    }
    return static_cast<ssize_t>(session.size_);
  }
  session.operation_ = session::operation::send;
  session.size_ = std::min(size, session.storage_.size());
  std::memcpy(session.storage_.data(), data, session.size_);
  return EAGAIN > 0 ? -EAGAIN : EAGAIN;
#else
  do {
    if (const auto rc = ::send(session.socket_.handle(), data, size, flags); rc >= 0) {
      return rc;
    }
  } while (errno == EINTR);
  session.operation_ = session::operation::send;
  return errno > 0 ? -errno : errno;
#endif
}

}  // namespace ice::net::ssh
