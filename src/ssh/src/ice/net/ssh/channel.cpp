#include "ice/net/ssh/channel.hpp"
#include <ice/net/ssh/error.hpp>
#include <ice/net/ssh/loop.hpp>
#include <libssh2.h>
#include <cstdlib>

#if ICE_DEBUG
#  include <ice/log.hpp>
#endif

namespace ice::net::ssh {

void channel::close_type::operator()(LIBSSH2_CHANNEL* handle) noexcept
{
  libssh2_channel_close(handle);
}

async<std::error_code> channel::close() noexcept
{
  const auto ec = co_await loop(session_, [&]() { return libssh2_channel_close(handle_); });
  handle_.reset();
  co_return ec;
}

async<std::error_code> channel::request_pty(std::string terminal) noexcept
{
  return loop(session_, [this, terminal = std::move(terminal)]() {
    return libssh2_channel_request_pty(handle_, terminal.data());
  });
}

async<std::error_code> channel::open_shell() noexcept
{
  return loop(session_, [this]() {
    return libssh2_channel_shell(handle_);
  });
}

async<int> channel::exec(std::string command, std::error_code& ec) noexcept
{
  ec = co_await loop(
    session_, [this, command = std::move(command)]() { return libssh2_channel_exec(handle_, command.data()); });
  co_return ec ? EXIT_FAILURE : libssh2_channel_get_exit_status(handle_);
}

async<std::size_t> channel::recv(FILE* stream, char* data, std::size_t size, std::error_code& ec) noexcept
{
  const auto stream_id = stream == stderr ? SSH_EXTENDED_DATA_STDERR : 0;
  while (true) {
    if (const auto rc = libssh2_channel_read_ex(handle_, stream_id, data, size); rc >= 0) {
      co_return static_cast<std::size_t>(rc);
    } else if (rc != LIBSSH2_ERROR_EAGAIN) {
      if (rc != LIBSSH2_ERROR_NONE) {
        ec = make_error_code(rc);
      }
      break;
    }
    co_await session_->io();
  }
  co_return{};
}

async<std::size_t> channel::send(FILE* stream, const char* data, std::size_t size, std::error_code& ec) noexcept
{
  const auto stream_id = stream == stderr ? SSH_EXTENDED_DATA_STDERR : 0;
  const auto data_size = size;
  do {
    if (const auto rc = libssh2_channel_write_ex(handle_, stream_id, data, size); rc > 0) {
      data += static_cast<std::size_t>(rc);
      size -= static_cast<std::size_t>(rc);
      continue;
    } else if (rc != LIBSSH2_ERROR_EAGAIN) {
      if (rc != LIBSSH2_ERROR_NONE) {
        ec = make_error_code(rc);
      }
      break;
    }
    co_await session_->io();
  } while (size > 0);
  co_return data_size - size;
}

}  // namespace ice::net::ssh
