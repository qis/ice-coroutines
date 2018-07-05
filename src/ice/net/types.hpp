#pragma once
#include <ice/config.hpp>

struct sockaddr;
struct sockaddr_in;
struct sockaddr_in6;

namespace ice::net {

#if ICE_OS_WIN32
using socklen_t = int;
#else
using socklen_t = unsigned int;
#endif

}  // namespace ice::net
