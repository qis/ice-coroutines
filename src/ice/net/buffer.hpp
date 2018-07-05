#pragma once
#include <ice/config.hpp>
#include <limits>
#include <cassert>
#include <cstddef>

namespace ice::net {

template <typename T>
struct basic_buffer {
#if ICE_OS_WIN32
  using size_type = unsigned long;
#else
  using size_type = std::size_t;
#endif
  using data_type = T*;

  constexpr basic_buffer(T* data = nullptr, std::size_t size = 0) noexcept :
    size(static_cast<size_type>(size)), data(data)
  {
    assert(size <= std::numeric_limits<size_type>::max());
    assert(size == 0 || data != nullptr);
  }

  size_type size;
  data_type data;
};

using buffer = basic_buffer<char>;
using const_buffer = basic_buffer<const char>;

}  // namespace ice::net
