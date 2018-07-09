#include "socket.hpp"

#if ICE_OS_WIN32
#  include <windows.h>
#  include <winsock2.h>
#else
#  include <sys/socket.h>
#  include <sys/types.h>
#  include <unistd.h>
#endif

namespace ice::net {

#if ICE_OS_WIN32

static_assert(INVALID_SOCKET == socket::handle_type::invalid_value());

void socket::close_type::operator()(std::uintptr_t handle) noexcept
{
  ::closesocket(static_cast<SOCKET>(handle));
}

#else

void socket::close_type::operator()(int handle) noexcept
{
  while (::close(handle) && errno == EINTR) {
  }
}

#endif

socket::socket(ice::service& service, int family, int type, int protocol) : service_(service)
{
  create(family, type, protocol);
}

void socket::create(int family, int type)
{
  create(family, type, 0);
}

void socket::create(int family, int type, std::error_code& ec) noexcept
{
  create(family, type, 0, ec);
}

void socket::create(int family, int type, int protocol)
{
  std::error_code ec;
  create(family, type, protocol, ec);
  throw_on_error(ec, "create socket");
}

void socket::create(int family, int type, int protocol, std::error_code& ec) noexcept
{
#if ICE_OS_WIN32
  handle_type handle{ ::WSASocketW(family, type, protocol, nullptr, 0, WSA_FLAG_OVERLAPPED) };
  if (!handle) {
    ec = make_error_code(::WSAGetLastError());
    return;
  }
  if (!::CreateIoCompletionPort(handle.as<HANDLE>(), service().handle().as<HANDLE>(), 0, 0)) {
    ec = make_error_code(::GetLastError());
    return;
  }
  if (!::SetFileCompletionNotificationModes(handle.as<HANDLE>(), FILE_SKIP_COMPLETION_PORT_ON_SUCCESS)) {
    ec = make_error_code(::GetLastError());
    return;
  }
#else
  handle_type handle{ ::socket(family, type | SOCK_NONBLOCK, protocol) };
  if (!handle) {
    ec = make_error_code(errno);
    return;
  }
#endif
  handle_ = std::move(handle);
}

void socket::bind(const net::endpoint& endpoint)
{
  std::error_code ec;
  bind(endpoint, ec);
  throw_on_error(ec, "bind socket");
}

void socket::bind(const net::endpoint& endpoint, std::error_code& ec) noexcept
{
#if ICE_OS_WIN32
  if (::bind(handle_, &endpoint.sockaddr(), endpoint.size()) == SOCKET_ERROR) {
    ec = make_error_code(::WSAGetLastError());
    return;
  }
#else
  while (::bind(handle_, &endpoint.sockaddr(), endpoint.size()) < 0) {
    if (errno != EINTR) {
      ec = make_error_code(errno);
      return;
    }
  }
#endif
  local_endpoint_ = endpoint;
}

void socket::shutdown()
{
  shutdown(net::shutdown::both);
}

void socket::shutdown(std::error_code& ec) noexcept
{
  shutdown(net::shutdown::both, ec);
}

void socket::shutdown(net::shutdown direction)
{
  std::error_code ec;
  shutdown(direction, ec);
  throw_error(ec, "socket shutdown");
}

void socket::shutdown(net::shutdown direction, std::error_code& ec) noexcept
{
#if ICE_OS_WIN32
  auto value = 0;
  switch (direction) {
  case net::shutdown::recv: value = SD_RECEIVE; break;
  case net::shutdown::send: value = SD_SEND; break;
  case net::shutdown::both: value = SD_BOTH; break;
  }
  if (::shutdown(handle_, value) == SOCKET_ERROR) {
    ec = make_error_code(::WSAGetLastError());
    return;
  }
#else
  auto value = 0;
  switch (direction) {
  case net::shutdown::recv: value = SHUT_RD; break;
  case net::shutdown::send: value = SHUT_WR; break;
  case net::shutdown::both: value = SHUT_RDWR; break;
  }
  if (::shutdown(handle_, value) < 0) {
    ec = make_error_code(errno);
    return;
  }
#endif
}

int socket::type() const noexcept
{
  auto data = 0;
  auto size = socklen_t(sizeof(data));
  std::error_code ec;
  get(SOL_SOCKET, SO_TYPE, &data, size, ec);
  if (ec) {
    return 0;
  }
  return data;
}

int socket::protocol() const noexcept
{
#if ICE_OS_WIN32
  WSAPROTOCOL_INFOW data = {};
  auto size = socklen_t(sizeof(data));
  std::error_code ec;
  get(SOL_SOCKET, SO_PROTOCOL_INFOW, &data, size, ec);
  if (ec) {
    return 0;
  }
  return data.iProtocol;
#else
  auto data = 0;
  auto size = socklen_t(sizeof(data));
  std::error_code ec;
  get(SOL_SOCKET, SO_PROTOCOL, &data, size, ec);
  if (ec) {
    return 0;
  }
  return data;
#endif
}

void socket::get(int level, int name, void* data, socklen_t& size, std::error_code& ec) const noexcept
{
#if ICE_OS_WIN32
  if (::getsockopt(handle_, level, name, reinterpret_cast<char*>(data), &size) == SOCKET_ERROR) {
    ec = make_error_code(::WSAGetLastError());
    return;
  }
#else
  if (::getsockopt(handle_, level, name, data, &size) < 0) {
    ec = make_error_code(errno);
    return;
  }
#endif
}

void socket::set(int level, int name, const void* data, socklen_t size, std::error_code& ec) noexcept
{
#if ICE_OS_WIN32
  if (::setsockopt(handle_, level, name, reinterpret_cast<const char*>(data), size) == SOCKET_ERROR) {
    ec = make_error_code(::WSAGetLastError());
    return;
  }
#else
  if (::setsockopt(handle_, level, name, data, size) < 0) {
    ec = make_error_code(errno);
    return;
  }
#endif
}

}  // namespace ice::net
