#pragma once
#include "password.hpp"
#include <ice/log.hpp>
#include <ice/net/endpoint.hpp>
#include <ice/net/service.hpp>
#include <ice/net/ssh/session.hpp>
#include <ice/state.hpp>

class firmware {
public:
  enum class state {
    login,
    done,
  };

  using async_state = ice::async<state>;

  // clang-format off

  firmware(ice::net::service& service) noexcept :
    session_(service), sm_(state::login)
  {}

  // clang-format on

  ice::async<std::error_code> exec(std::string command = {}, bool hidden = false) noexcept
  {
    std::error_code ec;
    if (!command.empty()) {
      if (hidden) {
        ice::log::info(ice::color::red, std::string(command.size(), '*'));
      } else {
        ice::log::info(ice::color::red, command);
      }
      co_await ice::send(channel_, command, ec);
      if (ec) {
        co_return ec;
      }
    }
    co_await ice::send(channel_, "\r\n", ec);
    co_return ec;
  }

  ice::async<std::error_code> run(ice::net::endpoint endpoint) noexcept
  {
    if (const auto ec = session_.create(endpoint.family())) {
      ice::log::error(ec, "could not create ssh session");
      co_return ec;
    }
    ice::log::info("connecting to {} ...", endpoint);
    if (const auto ec = co_await session_.connect(endpoint)) {
      ice::log::error(ec, "could not connect");
      co_return ec;
    }
    ice::log::info("authenticating ...");
    if (const auto ec = co_await session_.authenticate("admin", get_password())) {
      ice::log::error(ec, "username/password authentication failed");
      co_return ec;
    }
    ice::log::debug("ready");
    std::error_code ec;
    channel_ = co_await session_.open(ec);
    if (!channel_) {
      ice::log::error(ec, "could not open channel");
      co_return ec;
    }
    std::string buffer;
    buffer.resize(128);
    while (sm_.state() != state::done) {
      const auto size = co_await channel_.recv(buffer.data(), buffer.size(), ec);
      if (ec) {
        ice::log::error(ec, "ssh channel read error");
        co_return ec;
      }
      ec = co_await sm_.parse(buffer.data(), size);
      if (ec) {
        ice::log::error(ec, "state manager error");
        co_return ec;
      }
    }
    co_return{};
  }

private:
  struct info {
    std::string bootloader;
    std::string primary_image;
    std::string aireos;
    std::string mac;
    unsigned long max = 0;
  } info_;

  ice::net::ssh::session session_;
  ice::net::ssh::channel channel_;
  ice::state::manager<state> sm_;
};
