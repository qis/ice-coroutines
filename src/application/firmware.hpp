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
    parse_inventory,
    done,
  };

  using async_state = ice::async<state>;

  // clang-format off

  firmware(ice::net::service& service) noexcept :
    session_(service), sm_(state::login)
  {
    constexpr auto prompt = "\\s*\\([^\\)]*\\)\\s>\\s*";

    // ================================================================================================================

    // User:
    sm_.add(state::login, "User\\s*:\\s*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec("admin");
      co_return state::login;
    });

    // Password:
    sm_.add(state::login, "Password\\s*:\\s*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec(get_password(), true);
      co_return state::login;
    });

    // (Cisco Controller) >
    sm_.add(state::login, prompt, [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec("show inventory");
      co_return state::parse_inventory;
    });

    // Burned-in MAC Address............................ XX:XX:XX:XX:XX:XX
    sm_.add(state::parse_inventory, "Burned-in MAC Address[.\\s]*([^\\s]+).*", [&](auto& sm, auto& ec) -> async_state {
      info_.mac = sm[1].str();
      ice::log::notice("MAC: {}", info_.mac);
      co_return state::parse_inventory;
    });

    // Maximum number of APs supported.................. 150
    sm_.add(state::parse_inventory, "Maximum number of APs supported[.\\s]*(\\d+).*", [&](auto& sm, auto& ec) -> async_state {
      info_.max = std::stoul(sm[1].str());
      ice::log::notice("Max clients: {}", info_.max);
      co_return state::parse_inventory;
    });

    // DESCR: "Cisco 3500 Series Wireless LAN Controller"
    sm_.add(state::parse_inventory, ".*DESCR: \"([^\"]*)\".*",
      [&](auto& sm, auto& ec) -> async_state {
      ice::log::notice("Description: {}", sm[1].str());
      co_return state::parse_inventory;
    }, false);

    // PID: AIR-XXXXXX-XX
    sm_.add(state::parse_inventory, ".*PID:\\s*([^\\s,]*).*", [&](auto& sm, auto& ec) -> async_state {
      ice::log::notice("Product ID: {}", sm[1].str());
      co_return state::parse_inventory;
    }, false);

    // SN: XXXXXXXXXXX
    sm_.add(state::parse_inventory, ".*SN:\\s*([^\\s,]*).*", [&](auto& sm, auto& ec) -> async_state {
      ice::log::notice("Serial Number: {}", sm[1].str());
      co_return state::parse_inventory;
    }, false);

    // (Cisco Controller) >
    sm_.add(state::parse_inventory, prompt, [&](auto& sm, auto& ec) -> async_state {
      co_return state::done;
    });
  }

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
    co_await ice::send(channel_, "\r", ec);
    co_return ec;
  }

  ice::async<std::error_code> run(ice::net::endpoint endpoint) noexcept
  {
    ice::log::debug("creating session ...");
    if (const auto ec = session_.create(endpoint.family())) {
      ice::log::error(ec, "could not create ssh session");
      co_return ec;
    }
    ice::log::debug("connecting to {} ...", endpoint);
    if (const auto ec = co_await session_.connect(endpoint)) {
      ice::log::error(ec, "could not connect");
      co_return ec;
    }
    ice::log::debug("authenticating ...");
    if (const auto ec = co_await session_.authenticate("admin", get_password())) {
      ice::log::error(ec, "username/password authentication failed");
      co_return ec;
    }
    ice::log::debug("opening channel ...");
    {
      std::error_code ec;
      channel_ = co_await session_.open(ec);
      if (!channel_) {
        ice::log::error(ec, "could not open channel");
        co_return ec;
      }
    }
    ice::log::debug("request pty ...");
    if (const auto ec = co_await channel_.request_pty("vanilla")) {
      ice::log::error(ec, "could not request pty");
      co_return ec;
    }
    ice::log::debug("opening shell ...");
    if (const auto ec = co_await channel_.open_shell()) {
      ice::log::error(ec, "could not open shell");
      co_return ec;
    }
    ice::log::debug("ready");
    std::string buffer;
    buffer.resize(128);
    while (sm_.state() != state::done) {
      std::error_code ec;
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

  ice::async<void> close() noexcept
  {
    co_await channel_.close();
    co_await session_.disconnect();
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
