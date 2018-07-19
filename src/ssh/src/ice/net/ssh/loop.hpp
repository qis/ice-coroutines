#pragma once
#include <ice/async.hpp>
#include <ice/net/ssh/error.hpp>
#include <ice/net/ssh/session.hpp>
#include <libssh2.h>
#include <cassert>

namespace ice::net::ssh {

template <typename Function>
inline async<std::error_code> loop(session* session, Function function) noexcept
{
  assert(session);
  while (true) {
    const auto rc = function();
    if (rc == LIBSSH2_ERROR_NONE) {
      break;
    }
    if (rc != LIBSSH2_ERROR_EAGAIN) {
      co_return make_error_code(rc, domain_category());
    }
    co_await session->io();
  }
  co_return{};
}

}  // namespace ice::net::ssh
