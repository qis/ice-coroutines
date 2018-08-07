#pragma once
#include "password.hpp"
#include <ice/log.hpp>
#include <ice/net/serial/port.hpp>
#include <ice/net/service.hpp>
#include <ice/state.hpp>

#ifndef WLC_RESET
#  define WLC_RESET 1
#endif

class startup {
public:
  enum class state {
    boot,
    init,
    login,
    parse_inventory,
    done,
  };

  using async_state = ice::async<state>;

  // clang-format off

  startup(ice::net::service& service, std::string ip, std::string nm, std::string gw, unsigned vlan) noexcept :
    stream_(service), sm_(state::boot)
  {
    constexpr auto prompt = "\\s*\\([^\\)]*\\)\\s>\\s*";

    // ================================================================================================================

    // WLCNG Boot Loader Version 1.0.20 (Built on Jan  9 2014 at 19:02:44 by cisco)
    sm_.add(state::boot, "WLCNG Boot Loader Version\\s*([\\s]*).*", [&](auto& sm, auto& ec) -> async_state {
      info_.bootloader = sm[1].str();
      ice::log::notice("WLCNG Boot Loader Version: {}", info_.bootloader);
      co_return state::boot;
    });

    // Cisco BootLoader Version : 8.5.103.0 (Cisco build) (Build time: Jul 25 2017 - 07:47:10)
    sm_.add(state::boot, "Cisco BootLoader Version\\s*:\\s*([0-9\\.]*).*", [&](auto& sm, auto& ec) -> async_state {
      info_.bootloader = sm[1].str();
      ice::log::notice("Cisco BootLoader Version: {}", info_.bootloader);
      co_return state::boot;
    });

    // Loading primary image (8.5.105.0)
    sm_.add(state::boot, "Loading primary image\\s*\\(([^\\)]*).*", [&](auto& sm, auto& ec) -> async_state {
      info_.primary_image = sm[1].str();
      ice::log::notice("Primary Image Version: {}", info_.primary_image);
      co_return state::boot;
    });

    // Cisco AireOS Version 8.5.105.0
    sm_.add(state::boot, "Cisco AireOS Version\\s*([^\\s]*).*", [&](auto& sm, auto& ec) -> async_state {
      info_.aireos = sm[1].str();
      ice::log::notice("Cisco AireOS Version: {}", info_.primary_image);
      co_return state::boot;
    });

    // ================================================================================================================

    // Would you like to terminate autoinstall? [yes]:
    sm_.add(state::boot, "Would you like to terminate autoinstall\\?[^:]*.*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec("yes");
      co_return state::init;
    });

    // System Name [Cisco_XX:XX:XX] (31 characters max):
    sm_.add(state::init, "System Name[^:]*.*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec("wlc");
      co_return state::init;
    });

    // Enter Administrative User Name (24 characters max):
    sm_.add(state::init, "Enter Administrative User Name[^:]*.*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec("admin");
      co_return state::init;
    });

    // Enter Administrative Password (3 to 24 characters):
    sm_.add(state::init, "Enter Administrative Password[^:]*.*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec(get_password(), true);
      co_return state::init;
    });

    // Re-enter Administrative Password                 :
    sm_.add(state::init, "Re-enter Administrative Password[^:]*.*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec(get_password(), true);
      co_return state::init;
    });

    // Service Interface IP Address Configuration [static][DHCP]:
    sm_.add(state::init, "Service Interface IP Address Configuration[^:]*.*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec("static");
      co_return state::init;
    });

    // Service Interface IP Address:
    sm_.add(state::init, "Service Interface IP Address[^:]*.*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec("2.2.2.2");
      co_return state::init;
    });

    // Service Interface Netmask:
    sm_.add(state::init, "Service Interface Netmask[^:]*.*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec("255.255.255.0");
      co_return state::init;
    });

    // Enable Link Aggregation (LAG) [yes][NO]:
    sm_.add(state::init, "Enable Link Aggregation[^:]*.*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec("no");
      co_return state::init;
    });

    // mGig Port Max Speed [1000][2500][5000]:
    sm_.add(state::init, "mGig Port Max Speed([^:]*).*", [&](auto& sm, auto& ec) -> async_state {
      auto str = sm[1].str();
      const std::regex re{ "\\[(\\d+)\\]", std::regex_constants::ECMAScript | std::regex_constants::optimize };
      unsigned long value = 0;
      while (std::regex_search(str, sm, re)) {
        value = std::max(value, std::stoul(sm[1].str()));
        str = sm.suffix();
      }
      ec = co_await exec(std::to_string(value));
      co_return state::init;
    });

    // Management Interface IP Address:
    sm_.add(state::init, "Management Interface IP Address[^:]*.*", [=](auto& sm, auto& ec) -> async_state {
      ec = co_await exec(ip);
      co_return state::init;
    });

    // Management Interface Netmask:
    sm_.add(state::init, "Management Interface Netmask[^:]*.*", [=](auto& sm, auto& ec) -> async_state {
      ec = co_await exec(nm);
      co_return state::init;
    });

    // Management Interface Default Router:
    sm_.add(state::init, "Management Interface Default Router[^:]*.*", [=](auto& sm, auto& ec) -> async_state {
      ec = co_await exec(gw);
      co_return state::init;
    });

    // Management Interface VLAN Identifier (0 = untagged):
    sm_.add(state::init, "Management Interface VLAN Identifier[^:]*.*", [=](auto& sm, auto& ec) -> async_state {
      ec = co_await exec(std::to_string(vlan));
      co_return state::init;
    });

    // Management Interface Port Num [1 to 5]:
    sm_.add(state::init, "Management Interface Port Num[^:]*.*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec("1");
      co_return state::init;
    });

    // Management Interface DHCP Server IP Address:
    sm_.add(state::init, "Management Interface DHCP Server IP Address[^:]*.*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec("10.11.94.225");
      co_return state::init;
    });

    // Enable HA [yes][NO]:
    sm_.add(state::init, "Enable HA[^:]*.*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec("no");
      co_return state::init;
    });

    // Virtual Gateway IP Address:
    sm_.add(state::init, "Virtual Gateway IP Address[^:]*.*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec("1.1.1.1");
      co_return state::init;
    });

    // Multicast IP Address:
    sm_.add(state::init, "Multicast IP Address[^:]*.*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec("239.0.1.1");
      co_return state::init;
    });

    // Mobility/RF Group Name:
    sm_.add(state::init, "Mobility/RF Group Name[^:]*.*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec("default");
      co_return state::init;
    });

    // Network Name (SSID):
    sm_.add(state::init, "Network Name \\(SSID\\)[^:]*.*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec("default");
      co_return state::init;
    });

    // Configure DHCP Bridging Mode [yes][NO]:
    sm_.add(state::init, "Configure DHCP Bridging Mode[^:]*.*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec("no");
      co_return state::init;
    });

    // Allow Static IP Addresses [YES][no]:
    sm_.add(state::init, "Allow Static IP Addresses[^:]*.*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec("yes");
      co_return state::init;
    });

    // Configure a RADIUS Server now? [YES][no]:
    sm_.add(state::init, "Configure a RADIUS Server now\\?[^:]*.*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec("no");
      co_return state::init;
    });

    // Enter Country Code list (enter 'help' for a list of countries) [US]:
    sm_.add(state::init, "Enter Country Code list[^:]*.*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec("DE");
      co_return state::init;
    });

    // Enable 802.11b Network [YES][no]:
    // Enable 802.11a Network [YES][no]:
    sm_.add(state::init, "Enable [^\\s]+ Network[^:]*.*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec("no");
      co_return state::init;
    });

    // Enable Auto-RF [YES][no]:
    sm_.add(state::init, "Enable Auto-RF[^:]*.*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec("no");
      co_return state::init;
    });

    // Configure a NTP server now? [YES][no]:
    sm_.add(state::init, "Configure a NTP server now\\?[^:]*.*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec("yes");
      co_return state::init;
    });

    // Enter the NTP server's IP address:
    sm_.add(state::init, "Enter the NTP server's IP address[^:]*.*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec("10.11.94.225");
      co_return state::init;
    });

    // Enter a polling interval between 3600 and 604800 secs:
    sm_.add(state::init, "Enter a polling interval between 3600 and 604800 secs[^:]*.*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec("3600");
      co_return state::init;
    });

    // Would you like to configure IPv6 parameters[YES][no]:
    sm_.add(state::init, "Would you like to configure IPv6 parameters[^:]*.*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec("no");
      co_return state::init;
    });

    // Configuration correct? If yes, system will save it and reset. [yes][NO]:
    sm_.add(state::init, "Configuration correct\\?[^:]*.*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec("yes");
      co_return state::login;
    });

#if WLC_RESET
    // User:
    sm_.add(state::boot, "User\\s*:\\s*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec("Recover-Config");
      co_return state::boot;
    });
#else
    // User:
    sm_.add(state::boot, "User\\s*:\\s*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec("admin");  // 'admin' or 'Recover-Config'
      co_return state::login;
    });
#endif

    // ================================================================================================================

    // User:
    sm_.add(state::login, "User\\s*:\\s*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec("admin");
      co_return state::login;
    });

    // Password:
    sm_.add(state::login, "Password\\s*:\\s*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec(get_password(), true);
      co_return state::boot;
    });

    // (Cisco Controller) >
    sm_.add(state::boot, prompt, [&](auto& sm, auto& ec) -> async_state {
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
      co_await ice::send(stream_, command, ec);
      if (ec) {
        co_return ec;
      }
    }
    co_await ice::send(stream_, "\r\n", ec);
    co_return ec;
  }

  ice::async<std::error_code> run(unsigned port = 0) noexcept
  {
    if (const auto ec = stream_.open(port)) {
      ice::log::error(ec, "serial port open error");
      co_return ec;
    }
    std::string buffer;
    buffer.resize(128);
    while (sm_.state() != state::done) {
      std::error_code ec;
      const auto size = co_await stream_.recv(buffer.data(), buffer.size(), ec);
      if (ec) {
        ice::log::error(ec, "serial port read error");
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

  ice::net::serial::port stream_;
  ice::state::manager<state> sm_;
};
