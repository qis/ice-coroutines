#include "socket.hpp"
#include <cassert>

#if ICE_OS_WIN32
#  include <windows.h>
#  include <winsock2.h>
#  include <mswsock.h>
#  include <ws2tcpip.h>
#  include <new>
#else
#  include <netinet/in.h>
#  include <sys/socket.h>
#  include <sys/types.h>
#  include <unistd.h>
#endif

namespace ice::net::tcp {
namespace detail {

#if ICE_OS_WIN32

struct connect_ex {
  connect_ex()
  {
    net::socket::handle_type socket(::WSASocketW(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED));
    if (!socket) {
      ec = make_error_code(::WSAGetLastError());
      return;
    }
    auto fp = SIO_GET_EXTENSION_FUNCTION_POINTER;
    auto guid = GUID(WSAID_CONNECTEX);
    auto size = DWORD(sizeof(function_));
    if (::WSAIoctl(socket, fp, &guid, sizeof(guid), &function_, size, &size, nullptr, nullptr) == SOCKET_ERROR) {
      ec = make_error_code(::WSAGetLastError());
      return;
    }
  }

  template <typename... Args>
  auto operator()(Args&&... args) const noexcept
  {
    assert(function_);
    return function_(std::forward<Args>(args)...);
  }

  std::error_code ec;

private:
  LPFN_CONNECTEX function_ = nullptr;
};

#endif

}  // namespace detail

socket::socket(ice::service& service, int family) : net::socket(service, family, SOCK_STREAM, IPPROTO_TCP) {}

socket::socket(ice::service& service, int family, int protocol) : net::socket(service, family, SOCK_STREAM, protocol) {}

std::error_code socket::create(int family) noexcept
{
  return net::socket::create(family, SOCK_STREAM, IPPROTO_TCP);
}

std::error_code socket::create(int family, int protocol) noexcept
{
  return net::socket::create(family, SOCK_STREAM, protocol);
}

std::error_code socket::listen(std::size_t backlog)
{
  const auto size = backlog > 0 ? static_cast<int>(backlog) : SOMAXCONN;
#if ICE_OS_WIN32
  if (::listen(handle_, size) == SOCKET_ERROR) {
    return make_error_code(::WSAGetLastError());
  }
#else
  if (::listen(handle_, size) < 0) {
    return make_error_code(errno);
  }
#endif
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
  ::linger data = { 1, 0 };
  ::setsockopt(handle_, SOL_SOCKET, SO_LINGER, &data, sizeof(data));
#endif
  return {};
}

}  // namespace ice::net::tcp
