#pragma once
#include <ice/config.hpp>
#include <ice/net/types.hpp>
#include <chrono>
#include <limits>
#include <optional>
#include <type_traits>
#include <cstddef>

// NOTE: On Windows, SO_RCVTIMEO and SO_SNDTIMEO are not supported by IOCP sockets.
// TODO: See if this works on Linux and FreeBSD. Use IOCP to simulate this behavior on Windows.

//#if !ICE_OS_WIN32
//#  include <sys/time.h>
//#endif

namespace ice::net {

class option {
public:
  class broadcast;
  class do_not_route;
  class keep_alive;
  class linger;
  class no_delay;
  //class recv_timeout;
  //class send_timeout;
  class recv_buffer_size;
  class send_buffer_size;
  class recv_low_watermark;
  class send_low_watermark;
  class reuse_address;

  virtual ~option() = default;

  virtual int level() const noexcept;
  virtual int name() const noexcept = 0;

  virtual void* data() noexcept = 0;
  virtual const void* data() const noexcept = 0;

  virtual socklen_t capacity() const noexcept = 0;

  socklen_t& size() noexcept
  {
    return size_;
  }

  const socklen_t& size() const noexcept
  {
    return size_;
  }

protected:
  socklen_t size_ = 0;
};

template <typename T>
class option_value : public option {};

template <>
class option_value<bool> : public option {
public:
  option_value(bool value = false) noexcept
  {
    value_ = value ? 1 : 0;
    size_ = capacity();
  }

  bool get() const noexcept
  {
    return value_ ? true : false;
  }

  void* data() noexcept override
  {
    return &value_;
  }

  const void* data() const noexcept override
  {
    return &value_;
  }

  socklen_t capacity() const noexcept override
  {
    return sizeof(value_);
  }

protected:
  int value_ = 0;
};

template <>
class option_value<std::size_t> : public option {
public:
  option_value(std::size_t value = 0) noexcept
  {
    if (value > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
      value_ = std::numeric_limits<int>::max();
    } else {
      value_ = static_cast<int>(value);
    }
    size_ = capacity();
  }

  std::size_t get() const noexcept
  {
    return value_ > 0 ? static_cast<std::size_t>(value_) : 0;
  }

  void* data() noexcept override
  {
    return &value_;
  }

  const void* data() const noexcept override
  {
    return &value_;
  }

  socklen_t capacity() const noexcept override
  {
    return sizeof(value_);
  }

protected:
  int value_ = 0;
};

//template <>
//class option_value<std::chrono::microseconds> : public option {
//public:
//  option_value(std::chrono::microseconds value = std::chrono::microseconds{ 0 }) noexcept
//  {
//#if ICE_OS_WIN32
//    if (value > std::chrono::microseconds{ 0 } && value < std::chrono::milliseconds{ 1 }) {
//      value_ = 1;
//    } else {
//      value_ = static_cast<unsigned long>(std::chrono::floor<std::chrono::milliseconds>(value).count());
//    }
//#else
//    const auto s = std::chrono::floor<std::chrono::seconds>(value);
//    const auto us = std::chrono::duration_cast<std::chrono::milliseconds>(value - s);
//    value_.tv_sec = static_cast<decltype(value_.tv_sec)>(s.count());
//    value_.tv_usec = static_cast<decltype(value_.tv_usec)>(us.count());
//#endif
//    size_ = capacity();
//  }
//
//  std::chrono::microseconds get() const noexcept
//  {
//#if ICE_OS_WIN32
//    return std::chrono::milliseconds{ value_ };
//#else
//    const auto s = std::chrono::seconds{ value_.tv_sec };
//    return s + std::chrono::microseconds{ value_.rv_usec };
//#endif
//  }
//
//  void* data() noexcept override
//  {
//    return &value_;
//  }
//
//  const void* data() const noexcept override
//  {
//    return &value_;
//  }
//
//  socklen_t capacity() const noexcept override
//  {
//    return sizeof(value_);
//  }
//
//protected:
//#if ICE_OS_WIN32
//  unsigned long value_ = 0;
//#else
//  struct timeval value_ = {};
//#endif
//};

class option::broadcast : public option_value<bool> {
public:
  using option_value::option_value;
  int name() const noexcept override;
};

class option::do_not_route : public option_value<bool> {
public:
  using option_value::option_value;
  int name() const noexcept override;
};

class option::keep_alive : public option_value<bool> {
public:
  using option_value::option_value;
  int name() const noexcept override;
};

class option::linger : public option {
public:
  using value_type = std::aligned_storage_t<linger_size, linger_alignment>;

  linger(std::optional<std::chrono::seconds> timeout = {}) noexcept;

  linger(const linger& other) noexcept;
  linger& operator=(const linger& other) noexcept;

  virtual ~linger();

  int name() const noexcept override;

  std::optional<std::chrono::seconds> get() const noexcept;

  void* data() noexcept override
  {
    return &value_;
  }

  const void* data() const noexcept override
  {
    return &value_;
  }

  socklen_t capacity() const noexcept override
  {
    return sizeof(value_);
  }

protected:
  value_type value_;
};

class option::no_delay : public option_value<bool> {
public:
  using option_value::option_value;
  int level() const noexcept override;
  int name() const noexcept override;
};

//class option::recv_timeout : public option_value<std::chrono::microseconds> {
//public:
//  using option_value::option_value;
//  int name() const noexcept override;
//};

//class option::send_timeout : public option_value<std::chrono::microseconds> {
//public:
//  using option_value::option_value;
//  int name() const noexcept override;
//};

class option::recv_buffer_size : public option_value<std::size_t> {
public:
  using option_value::option_value;
  int name() const noexcept override;
};

class option::send_buffer_size : public option_value<std::size_t> {
public:
  using option_value::option_value;
  int name() const noexcept override;
};

class option::recv_low_watermark : public option_value<std::size_t> {
public:
  using option_value::option_value;
  int name() const noexcept override;
};

class option::send_low_watermark : public option_value<std::size_t> {
public:
  using option_value::option_value;
  int name() const noexcept override;
};

class option::reuse_address : public option_value<bool> {
public:
  using option_value::option_value;
  int name() const noexcept override;
};

}  // namespace ice::net
