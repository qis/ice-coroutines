#include "socket.hpp"
#include <ice/event.hpp>
#include <array>
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

std::error_code socket::create(int family) noexcept
{
  return net::socket::create(family, SOCK_STREAM, IPPROTO_TCP);
}

std::error_code socket::create(int family, int protocol) noexcept
{
  return net::socket::create(family, SOCK_STREAM, protocol);
}

std::error_code socket::listen(std::size_t backlog) noexcept
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

#if ICE_OS_WIN32

async<tcp::socket> socket::accept(net::endpoint& endpoint) noexcept
{
  constexpr static DWORD buffer_size = sockaddr_storage_size + 16;
  std::array<char, buffer_size * 2> buffer;
  DWORD bytes = 0;
  tcp::socket client{ service() };
  if (client.create(family(), protocol())) {
    co_return tcp::socket{ service() };
  }
  const auto server_handle = handle().as<HANDLE>();
  const auto server_socket = handle().as<SOCKET>();
  const auto client_socket = client.handle().as<SOCKET>();
  while (true) {
    event ev;
    if (::AcceptEx(server_socket, client_socket, &buffer, 0, buffer_size, buffer_size, &bytes, &ev)) {
      break;
    }
    auto rc = ::WSAGetLastError();
    if (rc == ERROR_IO_PENDING) {
      co_await ev;
      if (::GetOverlappedResult(server_handle, &ev, &bytes, FALSE)) {
        break;
      }
      rc = ::GetLastError();
    }
    if (rc != WSAECONNRESET) {
      co_return tcp::socket{ service() };
    }
  }
  const auto addr = reinterpret_cast<::sockaddr_storage*>(&buffer[buffer_size]);
  switch (addr->ss_family) {
  case AF_INET:
    endpoint.sockaddr_in() = *reinterpret_cast<::sockaddr_in*>(addr);
    endpoint.size() = sizeof(::sockaddr_in);
    break;
  case AF_INET6:
    endpoint.sockaddr_in6() = *reinterpret_cast<::sockaddr_in6*>(addr);
    endpoint.size() = sizeof(::sockaddr_in6);
    break;
  }
  co_return std::move(client);
}

async<std::error_code> socket::connect(const net::endpoint& endpoint) noexcept
{
  static const detail::connect_ex connect;
  if (connect.ec) {
    co_return connect.ec;
  }
  const auto client_handle = handle().as<HANDLE>();
  const auto client_socket = handle().as<SOCKET>();
  sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = 0;
  if (::bind(client_socket, reinterpret_cast<const SOCKADDR*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
    co_return make_error_code(::WSAGetLastError());
  }
  event ev;
  if (connect(client_socket, &endpoint.sockaddr(), endpoint.size(), nullptr, 0, nullptr, &ev)) {
    co_return std::error_code{};
  }
  if (const auto rc = ::WSAGetLastError(); rc != ERROR_IO_PENDING) {
    co_return make_error_code(rc);
  }
  co_await ev;
  DWORD bytes = 0;
  if (!::GetOverlappedResult(client_handle, &ev, &bytes, FALSE)) {
    co_return make_error_code(::GetLastError());
  }
  co_return std::error_code{};
}

async<std::size_t> socket::recv(char* data, std::size_t size) noexcept
{
  const auto handle = handle_.as<HANDLE>();
  const auto socket = handle_.as<SOCKET>();
  WSABUF buffer = { static_cast<ULONG>(size), data };
  DWORD bytes = 0;
  DWORD flags = 0;
  event ev;
  if (::WSARecv(socket, &buffer, 1, &bytes, &flags, &ev, nullptr) != SOCKET_ERROR) {
    co_return bytes;
  }
  if (::WSAGetLastError() != ERROR_IO_PENDING) {
    co_return 0;
  }
  co_await ev;
  if (!::GetOverlappedResult(handle, &ev, &bytes, FALSE)) {
    co_return 0;
  }
  co_return bytes;
}

async<std::size_t> socket::send(const char* data, std::size_t size) noexcept
{
  const auto handle = handle_.as<HANDLE>();
  const auto socket = handle_.as<SOCKET>();
  WSABUF buffer = { static_cast<ULONG>(size), const_cast<char*>(data) };
  DWORD bytes = 0;
  do {
    event ev;
    if (::WSASend(socket, &buffer, 1, &bytes, 0, &ev, nullptr) == SOCKET_ERROR) {
      if (::WSAGetLastError() != ERROR_IO_PENDING) {
        break;
      }
      co_await ev;
      if (!::GetOverlappedResult(handle, &ev, &bytes, FALSE)) {
        break;
      }
    }
    if (bytes == 0) {
      break;
    }
    buffer.buf += bytes;
    buffer.len -= bytes;
  } while (buffer.len > 0);
  co_return static_cast<std::size_t>(size - buffer.len);
}

#else

async<tcp::socket> socket::accept(net::endpoint& endpoint) noexcept
{
  tcp::socket client{ service() };
  while (true) {
    endpoint.size() = endpoint.capacity();
    client.handle().reset(::accept4(handle(), &endpoint.sockaddr(), &endpoint.size(), SOCK_NONBLOCK));
    if (client) {
      break;
    }
    if (errno == EINTR) {
      continue;
    }
    if (errno != EAGAIN) {
      break;
    }
    if (co_await event{ service(), handle(), ICE_EVENT_RECV }) {
      break;
    }
  }
  co_return std::move(client);
}

async<std::error_code> socket::connect(const net::endpoint& endpoint) noexcept
{
  while (true) {
    if (::connect(handle(), &endpoint.sockaddr(), endpoint.size()) == 0) {
      co_return std::error_code{};
    }
#  if ICE_OS_LINUX
    if (errno == EINTR) {
      continue;
    }
    if (errno != EINPROGRESS) {
      co_return make_error_code(errno);
    }
#  elif ICE_OS_FREEBSD
    if (errno != EINPROGRESS && errno != EINTR) {
      co_return make_error_code(errno);
    }
#  endif
    if (const auto ec = co_await event{ service(), handle(), ICE_EVENT_SEND }) {
      co_return ec;
    }
    auto code = 0;
    auto size = static_cast<socklen_t>(sizeof(code));
    if (const auto ec = get(SOL_SOCKET, SO_ERROR, &code, size)) {
      co_return ec;
    }
    if (code) {
      co_return make_error_code(code);
    }
    break;
  }
  co_return std::error_code{};
}

async<std::size_t> socket::recv(char* data, std::size_t size) noexcept
{
  while (true) {
    if (const auto rc = ::read(handle(), data, size); rc >= 0) {
      co_return static_cast<std::size_t>(rc);
    }
    if (errno == EINTR) {
      continue;
    }
    if (errno != EAGAIN) {
      co_return std::size_t{};
    }
    if (co_await event{ service(), handle(), ICE_EVENT_RECV }) {
      co_return std::size_t{};
    }
  }
  co_return std::size_t{};
}

async<std::size_t> socket::send(const char* data, std::size_t size) noexcept
{
  const auto data_size = size;
  do {
    if (const auto rc = ::write(handle(), data, size); rc > 0) {
      data += static_cast<std::size_t>(rc);
      size -= static_cast<std::size_t>(rc);
    } else if (rc == 0) {
      break;
    }
    if (errno == EINTR) {
      continue;
    }
    if (errno != EAGAIN) {
      break;
    }
    if (co_await event{ service(), handle(), ICE_EVENT_SEND }) {
      break;
    }
  } while (size > 0);
  co_return data_size - size;
}

#endif

}  // namespace ice::net::tcp
