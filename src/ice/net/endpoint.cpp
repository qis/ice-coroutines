#include "endpoint.hpp"
#include <ice/error.hpp>
#include <new>

#if ICE_OS_WIN32
#  include <windows.h>
#  include <winsock2.h>
#  include <ws2tcpip.h>
#else
#  include <arpa/inet.h>
#  include <netinet/in.h>
#  include <sys/socket.h>
#  include <sys/types.h>
#endif

namespace ice::net {

//
// Using placement-new and manually calling the destructor is the correct thing to do
// and avoids undefined behavior. However, it is completely useless on any supported
// platform and compiler since undefined behavior works in practice and is faster.
//
// endpoint::endpoint() noexcept
// {
//   new (&storage_) sockaddr_storage;
// }
//
// endpoint::endpoint(const endpoint& other) noexcept : size_(other.size_)
// {
//   new (static_cast<void*>(&storage_)) sockaddr_storage{ reinterpret_cast<const sockaddr_storage&>(other.storage_) };
// }
//
// endpoint& endpoint::operator=(const endpoint& other) noexcept
// {
//   reinterpret_cast<sockaddr_storage&>(storage_) = reinterpret_cast<const sockaddr_storage&>(other.storage_);
//   size_ = other.size_;
//   return *this;
// }
//
// endpoint::~endpoint()
// {
//   reinterpret_cast<sockaddr_storage&>(storage_).~sockaddr_storage();
// }
//

endpoint::endpoint(const endpoint& other) noexcept : size_(other.size_)
{
  reinterpret_cast<sockaddr_storage&>(storage_) = reinterpret_cast<const sockaddr_storage&>(other.storage_);
}

endpoint& endpoint::operator=(const endpoint& other) noexcept
{
  reinterpret_cast<sockaddr_storage&>(storage_) = reinterpret_cast<const sockaddr_storage&>(other.storage_);
  size_ = other.size_;
  return *this;
}

std::error_code endpoint::create(const std::string& host, std::uint16_t port) noexcept
{
  int error = 0;
  if (host.find(":", 0, 5) == std::string::npos) {
    auto& addr = sockaddr_in();
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    size_ = sizeof(::sockaddr_in);
    error = ::inet_pton(AF_INET, host.data(), &addr.sin_addr);
  } else {
    auto& addr = sockaddr_in6();
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);
    size_ = sizeof(::sockaddr_in6);
    error = ::inet_pton(AF_INET6, host.data(), &addr.sin6_addr);
  }
  if (error != 1) {
    size_ = 0;
    switch (error) {
#if ICE_OS_WIN32
    case -1: return make_error_code(::WSAGetLastError());
#else
    case -1: return make_error_code(errno);
#endif
    default: return make_error_code(errc::invalid_address);
    }
  }
  return {};
}

std::string endpoint::host() const
{
  std::string buffer;
  switch (size_) {
  case sizeof(::sockaddr_in):
    buffer.resize(INET_ADDRSTRLEN);
    if (!inet_ntop(AF_INET, &sockaddr_in().sin_addr, buffer.data(), static_cast<socklen_t>(buffer.size()))) {
      return {};
    }
    break;
  case sizeof(::sockaddr_in6):
    buffer.resize(INET6_ADDRSTRLEN);
    if (!inet_ntop(AF_INET6, &sockaddr_in6().sin6_addr, buffer.data(), static_cast<socklen_t>(buffer.size()))) {
      return {};
    }
    break;
  default: return {};
  }
#if ICE_OS_WIN32
  buffer.resize(::strnlen_s(buffer.data(), buffer.size()));
#else
  buffer.resize(::strnlen(buffer.data(), buffer.size()));
#endif
  return buffer;
}

std::uint16_t endpoint::port() const noexcept
{
  switch (size_) {
  case sizeof(::sockaddr_in): return ntohs(sockaddr_in().sin_port);
  case sizeof(::sockaddr_in6): return ntohs(sockaddr_in6().sin6_port);
  }
  return 0;
}

int endpoint::family() const noexcept
{
  switch (size_) {
  case sizeof(::sockaddr_in): return AF_INET;
  case sizeof(::sockaddr_in6): return AF_INET6;
  }
  return 0;
}

}  // namespace ice::net
