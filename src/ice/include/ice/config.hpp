#pragma once
#include <fmt/core.h>

#if defined(WIN32)
#  define ICE_OS_WIN32 1
#elif defined(__linux__)
#  define ICE_OS_LINUX 1
#elif defined(__FreeBSD__)
#  define ICE_OS_FREEBSD 1
#else
#  error Unsupported OS
#endif

#ifndef ICE_OS_WIN32
#  define ICE_OS_WIN32 0
#endif

#ifndef ICE_OS_LINUX
#  define ICE_OS_LINUX 0
#endif

#ifndef ICE_OS_FREEBSD
#  define ICE_OS_FREEBSD 0
#endif

#ifndef ICE_EXCEPTIONS
#  ifdef _MSC_VER
#    if defined(_HAS_EXCEPTIONS) && !_HAS_EXCEPTIONS
#      define ICE_EXCEPTIONS 0
#    endif
#  else
#    if !defined(__cpp_exceptions) || !__cpp_exceptions
#      define ICE_EXCEPTIONS 0
#    endif
#  endif
#endif

#ifndef ICE_EXCEPTIONS
#  define ICE_EXCEPTIONS 1
#endif

#ifndef ICE_NO_EXCEPTIONS
#  define ICE_NO_EXCEPTIONS (!ICE_EXCEPTIONS)
#endif

#ifndef ICE_DEBUG
#  ifdef NDEBUG
#    define ICE_DEBUG 0
#  else
#    define ICE_DEBUG 1
#  endif
#endif

#ifndef ICE_NO_DEBUG
#  define ICE_NO_DEBUG (!ICE_DEBUG)
#endif

namespace fmt {
inline namespace v5 {
template <typename T, typename Char, typename Enable>
struct formatter;

}  // namespace v5
}  // namespace fmt
