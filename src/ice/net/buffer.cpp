#include "buffer.hpp"
#include <ice/utility.hpp>

#if ICE_OS_WIN32
#  include <windows.h>
#  include <winsock2.h>
#  include <type_traits>
#  include <cstddef>
#endif

namespace ice::net {

#if ICE_OS_WIN32

// Verify size.
static_assert(sizeof(buffer) == sizeof(WSABUF));

// Verify alignment.
static_assert(alignof(buffer) == alignof(WSABUF));

// Verify size type.
using buffer_size_type = decltype(buffer::size);
using wsabuf_size_type = decltype(WSABUF::len);
static_assert(std::is_same_v<buffer_size_type, wsabuf_size_type>);

// Verify data type.
using buffer_data_type = std::remove_const_t<std::remove_pointer_t<decltype(buffer::data)>>;
using wsabuf_data_type = std::remove_pointer_t<decltype(WSABUF::buf)>;
static_assert(std::is_same_v<buffer_data_type, wsabuf_data_type>);

// Verify size offset.
static_assert(ICE_OFFSETOF(buffer, size) == ICE_OFFSETOF(WSABUF, len));

// Verify data offset.
static_assert(ICE_OFFSETOF(buffer, data) == ICE_OFFSETOF(WSABUF, buf));

#endif

}  // namespace ice::net
