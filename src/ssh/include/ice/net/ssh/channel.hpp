#pragma once
#include <ice/async.hpp>
#include <ice/config.hpp>
#include <ice/handle.hpp>
#include <ice/net/ssh/types.hpp>
#include <cstdio>

namespace ice::net::ssh {

class session;

class channel {
public:
  struct close_type {
    void operator()(LIBSSH2_CHANNEL* handle) noexcept;
  };
  using handle_type = handle<LIBSSH2_CHANNEL*, nullptr, close_type>;
  using handle_view = handle_type::view;

  channel() noexcept = default;
  channel(session* session, handle_type handle) noexcept : session_(session), handle_(std::move(handle)) {}

  channel(channel&& other) = default;
  channel(const channel& other) = delete;
  channel& operator=(channel&& other) = default;
  channel& operator=(const channel& other) = delete;

  ~channel();

  explicit operator bool() const noexcept
  {
    return session_ && handle_;
  }

  async<std::error_code> close() noexcept;

  async<std::error_code> exec(std::string command) noexcept;

  async<std::size_t> recv(FILE* stream, char* data, std::size_t size, std::error_code& ec) noexcept;
  async<std::size_t> send(FILE* stream, const char* data, std::size_t size, std::error_code& ec) noexcept;

private:
  session* session_ = nullptr;
  handle_type handle_;
};

}  // namespace ice::net::ssh
