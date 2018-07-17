#include "ice/net/option.hpp"
#include <ice/error.hpp>
#include <new>

#if ICE_OS_WIN32
#  include <windows.h>
#  include <winsock2.h>
#else
#  include <netinet/tcp.h>
#  include <sys/socket.h>
#  include <sys/types.h>
#endif

namespace ice::net {

static_assert(linger_size == sizeof(::linger));
static_assert(linger_alignment == alignof(::linger));

int option::level() const noexcept
{
  return SOL_SOCKET;
}

int option::broadcast::name() const noexcept
{
  return SO_BROADCAST;
}

int option::do_not_route::name() const noexcept
{
  return SO_DONTROUTE;
}

int option::keep_alive::name() const noexcept
{
  return SO_KEEPALIVE;
}

option::linger::linger(std::optional<std::chrono::seconds> timeout) noexcept
{
  new (static_cast<void*>(&value_))::linger{};
  auto& data = reinterpret_cast<::linger&>(value_);
  if (timeout) {
    data.l_onoff = 1;
    if (timeout->count() < std::numeric_limits<decltype(data.l_linger)>::min()) {
      data.l_linger = 0;
    }
    if (timeout->count() > std::numeric_limits<decltype(data.l_linger)>::max()) {
      data.l_linger = std::numeric_limits<decltype(data.l_linger)>::max();
    }
    data.l_linger = static_cast<decltype(data.l_linger)>(timeout->count());
  }
}

option::linger::linger(const linger& other) noexcept
{
  new (static_cast<void*>(&value_))::linger{ reinterpret_cast<const ::linger&>(other.value_) };
}

option::linger& option::linger::operator=(const linger& other) noexcept
{
  reinterpret_cast<::linger&>(value_) = reinterpret_cast<const ::linger&>(other.value_);
  return *this;
}

option::linger::~linger()
{
  reinterpret_cast<::linger&>(value_).~linger();
}

int option::linger::name() const noexcept
{
  return SO_LINGER;
}

std::optional<std::chrono::seconds> option::linger::get() const noexcept
{
  const auto& data = reinterpret_cast<const ::linger&>(value_);
  if (data.l_onoff) {
    return std::chrono::seconds(data.l_linger);
  }
  return std::nullopt;
}

int option::no_delay::level() const noexcept
{
#if ICE_OS_WIN32
  return IPPROTO_TCP;
#else
  return SOL_SOCKET;
#endif
}

int option::no_delay::name() const noexcept
{
  return TCP_NODELAY;
}

int option::recv_buffer_size::name() const noexcept
{
  return SO_RCVBUF;
}

int option::send_buffer_size::name() const noexcept
{
  return SO_SNDBUF;
}

int option::recv_low_watermark::name() const noexcept
{
  return SO_RCVLOWAT;
}

int option::send_low_watermark::name() const noexcept
{
  return SO_SNDLOWAT;
}

int option::reuse_address::name() const noexcept
{
  return SO_REUSEADDR;
}

}  // namespace ice::net
