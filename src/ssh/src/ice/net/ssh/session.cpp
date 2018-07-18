#include "ice/net/ssh/session.hpp"
#include <ice/net/event.hpp>
#include <ice/net/ssh/error.hpp>
#include <libssh2.h>

#if ICE_OS_WIN32
#  include <windows.h>
#  include <winsock2.h>
#else
#  include <sys/socket.h>
#  include <sys/types.h>
#endif

#include <ice/log.hpp>

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

template <typename Function>
inline async<std::error_code> loop(session* session, Function function) noexcept
{
  while (true) {
    const auto rv = function();
    if (rv == LIBSSH2_ERROR_NONE) {
      break;
    }
    if (rv != LIBSSH2_ERROR_EAGAIN) {
      co_return make_error_code(rv, domain_category());
    }
    co_await session->io();
  }
  co_return{};
}

}  // namespace

void session::close_type::operator()(LIBSSH2_SESSION* handle) noexcept
{
  libssh2_session_set_blocking(handle, 1);
  libssh2_session_callback_set(handle, LIBSSH2_CALLBACK_RECV, nullptr);
  libssh2_session_callback_set(handle, LIBSSH2_CALLBACK_SEND, nullptr);
  libssh2_session_free(handle);
}

session::session(session&& other) noexcept : socket_(std::move(other.socket_)), handle_(std::move(other.handle_))
{
  operation_ = other.operation_;
#if ICE_OS_WIN32
  storage_ = other.storage_;
  size_ = other.size_;
  ready_ = other.ready_;
#endif
  *libssh2_session_abstract(handle_) = this;
}

session& session::operator=(session&& other) noexcept
{
  socket_ = std::move(other.socket_);
  handle_ = std::move(other.handle_);
  operation_ = other.operation_;
#if ICE_OS_WIN32
  storage_ = other.storage_;
  size_ = other.size_;
  ready_ = other.ready_;
#endif
  *libssh2_session_abstract(handle_) = this;
  return *this;
}

session::~session()
{
  if (connected_) {
    [](session session) noexcept -> task {
      co_await session.disconnect();
      ice::log::debug("disconnect");
    }(std::move(*this));
  }
}

std::error_code session::create(int family) noexcept
{
  struct library {
    library()
    {
      if (const auto rv = libssh2_init(0)) {
        ec = make_error_code(rv, domain_category());
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
    return library.ec;
  }
  tcp::socket socket{ service() };
  if (const auto ec = socket.create(family)) {
    return ec;
  }
  handle_type handle(libssh2_session_init());
  if (!handle) {
    return make_error_code(LIBSSH2_ERROR_ALLOC, domain_category());
  }
  if (const auto rv = libssh2_session_flag(handle, LIBSSH2_FLAG_COMPRESS, 1)) {
    return make_error_code(rv, domain_category());
  }
  *libssh2_session_abstract(handle) = this;
  libssh2_session_callback_set(handle, LIBSSH2_CALLBACK_RECV, reinterpret_cast<void*>(&recv_callback));
  libssh2_session_callback_set(handle, LIBSSH2_CALLBACK_SEND, reinterpret_cast<void*>(&send_callback));
  libssh2_session_set_blocking(handle, 0);
  handle_ = std::move(handle);
  socket_ = std::move(socket);
  return {};
}

async<std::error_code> session::connect(net::endpoint endpoint) noexcept
{
  if (connected_) {
    co_await disconnect();
  }
  if (const auto ec = co_await socket_.connect(endpoint)) {
    co_return ec;
  }
  if (const auto ec = co_await loop(this, [&]() { return libssh2_session_handshake(handle_, socket_.handle()); })) {
    co_return ec;
  }
  connected_ = true;
  co_return{};
}

async<void> session::disconnect() noexcept
{
  connected_ = false;
  co_await loop(this, [&]() { return libssh2_session_disconnect(handle_, "shutdown"); });
  socket_.close();
  co_return;
}

async<std::error_code> session::authenticate(std::string username, std::string password) noexcept
{
  return loop(this, [&]() { return libssh2_userauth_password(handle_, username.data(), password.data()); });
}

async<channel> session::open() noexcept
{
  while (true) {
    const auto handle = libssh2_channel_open_session(handle_);
    if (handle) {
      co_return{ this, ssh::channel::handle_type{ handle } };
    }
    if (const auto rc = libssh2_session_last_error(handle_, nullptr, nullptr, 0); rc != LIBSSH2_ERROR_EAGAIN) {
      break;
    }
    co_await io();
  }
  co_return{};
}

async<std::error_code> session::io() noexcept
{
  std::error_code ec;
#if ICE_OS_WIN32
  switch (operation_) {
  case operation::recv: size_ = co_await socket_.recv(storage_.data(), size_); break;
  case operation::send: size_ = co_await socket_.send(storage_.data(), size_); break;
  default: ec = make_error_code(std::errc::invalid_argument);
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
