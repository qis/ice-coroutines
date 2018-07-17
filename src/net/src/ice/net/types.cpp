#include "ice/net/types.hpp"

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

static_assert(std::is_same_v<socklen_t, ::socklen_t>);
static_assert(sockaddr_storage_size == sizeof(::sockaddr_storage));
static_assert(sockaddr_storage_alignment == alignof(::sockaddr_storage));

}  // namespace ice::net
