#pragma once
#include <ice/config.hpp>
#include <ice/net/types.hpp>
#include <string>
#include <system_error>
#include <type_traits>
#include <cstdint>

namespace ice::net {

extern const int ipv4;
extern const int ipv6;

class endpoint {
public:
  using storage_type = std::aligned_storage_t<sockaddr_storage_size, sockaddr_storage_alignment>;

  endpoint() noexcept = default;

  endpoint(const endpoint& other) noexcept;
  endpoint& operator=(const endpoint& other) noexcept;

  std::error_code create(const std::string& host, std::uint16_t port) noexcept;

  std::string host() const;
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

  constexpr static socklen_t capacity() noexcept
  {
    return sizeof(storage_type);
  }

private:
  storage_type storage_;
  socklen_t size_ = 0;
};

}  // namespace ice::net

namespace fmt {

template <>
struct formatter<ice::net::endpoint, char, void> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const ice::net::endpoint& ep, FormatContext& ctx)
  {
    return format_to(ctx.begin(), "{}:{}", ep.host(), ep.port());
  }
};

}  // namespace fmt
