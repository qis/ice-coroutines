#pragma once
#include <ice/config.hpp>
#include <ice/net/types.hpp>
#include <optional>
#include <string>
#include <system_error>
#include <type_traits>
#include <cstdint>

namespace ice::net {

class endpoint {
public:
  using storage_type = std::aligned_storage_t<sockaddr_storage_size, sockaddr_storage_alignment>;

  endpoint() noexcept;
  endpoint(const std::string& host, std::uint16_t port);

  endpoint(const endpoint& other) noexcept;
  endpoint& operator=(const endpoint& other) noexcept;

  ~endpoint();

  std::error_code create(const std::string& host, std::uint16_t port) noexcept;
  std::optional<std::string> host() const;
  std::uint16_t port() const noexcept;

  int family() const noexcept;

  constexpr void clear() noexcept
  {
    size_ = 0;
  }

  constexpr socklen_t& size() noexcept
  {
    return size_;
  }

  constexpr const socklen_t& size() const noexcept
  {
    return size_;
  }

  ::sockaddr& sockaddr() noexcept
  {
    return reinterpret_cast<::sockaddr&>(storage_);
  }

  const ::sockaddr& sockaddr() const noexcept
  {
    return reinterpret_cast<const ::sockaddr&>(storage_);
  }

  ::sockaddr_in& sockaddr_in() noexcept
  {
    return reinterpret_cast<::sockaddr_in&>(storage_);
  }

  const ::sockaddr_in& sockaddr_in() const noexcept
  {
    return reinterpret_cast<const ::sockaddr_in&>(storage_);
  }

  ::sockaddr_in6& sockaddr_in6() noexcept
  {
    return reinterpret_cast<::sockaddr_in6&>(storage_);
  }

  const ::sockaddr_in6& sockaddr_in6() const noexcept
  {
    return reinterpret_cast<const ::sockaddr_in6&>(storage_);
  }

  constexpr socklen_t capacity() noexcept
  {
    return sizeof(storage_);
  }

private:
  storage_type storage_;
  socklen_t size_ = 0;
};

}  // namespace ice::net
