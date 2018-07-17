#include "ice/net/socket.hpp"

#if ICE_OS_WIN32
#  include <windows.h>
#  include <winsock2.h>
#else
#  include <sys/socket.h>
#  include <sys/types.h>
#  include <unistd.h>
#endif

#include <ice/log.hpp>

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

std::error_code socket::create(int family, int type, int protocol) noexcept
{
#if ICE_OS_WIN32
  handle_type handle{ ::WSASocketW(family, type, protocol, nullptr, 0, WSA_FLAG_OVERLAPPED) };
  if (!handle) {
    return make_error_code(::WSAGetLastError());
  }
  if (!::CreateIoCompletionPort(handle.as<HANDLE>(), service().handle().as<HANDLE>(), 0, 0)) {
    return make_error_code(::GetLastError());
  }
  if (!::SetFileCompletionNotificationModes(handle.as<HANDLE>(), FILE_SKIP_COMPLETION_PORT_ON_SUCCESS)) {
    return make_error_code(::GetLastError());
  }
#else
  handle_type handle{ ::socket(family, type | SOCK_NONBLOCK, protocol) };
  if (!handle) {
    return make_error_code(errno);
  }
#endif
  handle_ = std::move(handle);
  return {};
}

std::error_code socket::bind(const endpoint& endpoint) noexcept
{
#if ICE_OS_WIN32
  if (::bind(handle_, &endpoint.sockaddr(), endpoint.size()) == SOCKET_ERROR) {
    return make_error_code(::WSAGetLastError());
  }
#else
  while (::bind(handle_, &endpoint.sockaddr(), endpoint.size()) < 0) {
    if (errno != EINTR) {
      return make_error_code(errno);
    }
  }
#endif
  return {};
}

std::error_code socket::shutdown(net::shutdown direction) noexcept
{
#if ICE_OS_WIN32
  auto value = 0;
  switch (direction) {
  case net::shutdown::recv: value = SD_RECEIVE; break;
  case net::shutdown::send: value = SD_SEND; break;
  case net::shutdown::both: value = SD_BOTH; break;
  }
  if (::shutdown(handle_, value) == SOCKET_ERROR) {
    return make_error_code(::WSAGetLastError());
  }
#else
  auto value = 0;
  switch (direction) {
  case net::shutdown::recv: value = SHUT_RD; break;
  case net::shutdown::send: value = SHUT_WR; break;
  case net::shutdown::both: value = SHUT_RDWR; break;
  }
  if (::shutdown(handle_, value) < 0) {
    return make_error_code(errno);
  }
#endif
  return {};
}

int socket::type() const noexcept
{
  auto data = 0;
  auto size = socklen_t(sizeof(data));
  std::error_code ec;
  if (get(SOL_SOCKET, SO_TYPE, &data, size)) {
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
  if (get(SOL_SOCKET, SO_PROTOCOL_INFOW, &data, size)) {
    return 0;
  }
  return data.iProtocol;
#else
  auto data = 0;
  auto size = socklen_t(sizeof(data));
  std::error_code ec;
  if (get(SOL_SOCKET, SO_PROTOCOL, &data, size)) {
    return 0;
  }
  return data;
#endif
}

endpoint socket::name() const noexcept
{
  endpoint endpoint;
  endpoint.size() = endpoint.capacity();
  if (::getsockname(handle(), &endpoint.sockaddr(), &endpoint.size()) != 0) {
    return {};
  }
  return endpoint;
}

std::error_code socket::get(int level, int name, void* data, socklen_t& size) const noexcept
{
#if ICE_OS_WIN32
  if (::getsockopt(handle_, level, name, reinterpret_cast<char*>(data), &size) == SOCKET_ERROR) {
    return make_error_code(::WSAGetLastError());
  }
#else
  if (::getsockopt(handle_, level, name, data, &size) < 0) {
    return make_error_code(errno);
  }
#endif
  return {};
}

std::error_code socket::set(int level, int name, const void* data, socklen_t size) noexcept
{
#if ICE_OS_WIN32
  if (::setsockopt(handle_, level, name, reinterpret_cast<const char*>(data), size) == SOCKET_ERROR) {
    return make_error_code(::WSAGetLastError());
  }
#else
  if (::setsockopt(handle_, level, name, data, size) < 0) {
    return make_error_code(errno);
  }
#endif
  return {};
}

}  // namespace ice::net
