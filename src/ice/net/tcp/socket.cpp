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

void socket::create(int family)
{
  create(family, IPPROTO_TCP);
}

void socket::create(int family, std::error_code& ec) noexcept
{
  create(family, IPPROTO_TCP, ec);
}

void socket::create(int family, int protocol)
{
  std::error_code ec;
  create(family, protocol, ec);
  throw_on_error(ec, "create tcp socket");
}

void socket::create(int family, int protocol, std::error_code& ec) noexcept
{
  net::socket::create(family, SOCK_STREAM, protocol, ec);
}

void socket::listen()
{
  listen(0);
}

void socket::listen(std::error_code& ec) noexcept
{
  listen(0, ec);
}

void socket::listen(std::size_t backlog)
{
  std::error_code ec;
  listen(backlog, ec);
  throw_on_error(ec, "listen on tcp socket");
}

void socket::listen(std::size_t backlog, std::error_code& ec) noexcept
{
  const auto size = backlog > 0 ? static_cast<int>(backlog) : SOMAXCONN;
#if ICE_OS_WIN32
  if (::listen(handle_, size) == SOCKET_ERROR) {
    ec = make_error_code(::WSAGetLastError());
    return;
  }
#else
  if (::listen(handle_, size) < 0) {
    ec = make_error_code(errno);
    return;
  }
#endif
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
  ::linger data = { 1, 0 };
  ::setsockopt(handle_, SOL_SOCKET, SO_LINGER, &data, sizeof(data));
#endif
}

accept::accept(socket& socket, std::error_code* ec) noexcept : socket_(socket), client_(socket.service()), uec_(ec)
{
#if ICE_OS_WIN32
  client_.create(socket_.family(), socket_.protocol(), ec_);
#endif
}

bool accept::await_ready() noexcept
{
  return true;
}

bool accept::await_suspend(std::experimental::coroutine_handle<> awaiter) noexcept
{
  awaiter_ = awaiter;
  return true;
}

void accept::resume() noexcept
{
  if (await_ready() || !await_suspend(awaiter_)) {
    awaiter_.resume();
  }
}

}  // namespace ice::net::tcp
