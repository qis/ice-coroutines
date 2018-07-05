#include <cstddef>

#ifdef WIN32
#  ifdef _MSC_VER
#    pragma warning(push, 0)
#  endif
#  define NOMINMAX 1
#  define WIN32_LEAN_AND_MEAN 1
#  include <windows.h>
#  include <winsock2.h>
#  ifdef _MSC_VER
#    pragma warning(pop)
#  endif
#  define CHECK_OS_WIN32 1
#else
#  include <sys/types.h>
#  include <sys/socket.h>
#  ifdef __linux__
#    include <sys/epoll.h>
#    define CHECK_OS_LINUX 1
#  endif
#  ifdef __FreeBSD__
#    include <sys/event.h>
#    define CHECK_OS_FREEBSD 1
#  endif
#endif

#ifndef CHECK_OS_WIN32
#  define CHECK_OS_WIN32 0
#endif

#ifndef CHECK_OS_LINUX
#  define CHECK_OS_LINUX 0
#endif

#ifndef CHECK_OS_FREEBSD
#  define CHECK_OS_FREEBSD 0
#endif

#if CHECK_OS_WIN32
struct native_event : OVERLAPPED {};
#elif CHECK_OS_LINUX
struct native_event : epoll_event {};
#elif CHECK_OS_FREEBSD
using native_event_base = struct kevent;
struct native_event : native_event_base {};
#endif

template <typename T, std::size_t Size = sizeof(T), std::size_t Alignment = alignof(T)>
struct check;

int main() {
  check<native_event>{};
  check<sockaddr_storage>{};
  check<linger>{};
}
