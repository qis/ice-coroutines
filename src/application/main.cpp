#include <ice/log.hpp>
#include <ice/net/serial/port.hpp>
#include <ice/net/service.hpp>
#include <ice/utility.hpp>
#include <functional>
#include <map>
#include <regex>
#include <string>
#include <vector>
#include <cstdlib>

template <typename State>
class handler {
public:
  using regex_flag_type = std::regex::flag_type;
  using match_flag_type = std::regex_constants::match_flag_type;

  constexpr static regex_flag_type default_icase_flags(bool icase) noexcept
  {
    return icase ? std::regex_constants::icase : static_cast<regex_flag_type>(0);
  }

  constexpr static match_flag_type default_eol_flags(bool eol) noexcept
  {
    return eol ? static_cast<match_flag_type>(0) : std::regex_constants::match_not_eol;
  }

  template <typename Handler>
  handler(const std::string& regex, Handler&& handler) : handler(regex, false, std::forward<Handler>(handler))
  {}

  template <typename Handler>
  handler(const std::string& regex, bool icase, Handler&& handler) :
    regex_(regex, std::regex_constants::ECMAScript | default_icase_flags(icase) | std::regex_constants::optimize),
    handler_(std::forward<Handler>(handler))
  {}

  bool match(const std::string& string, std::smatch& sm, bool eol) const noexcept
  {
    return std::regex_match(string, sm, regex_, default_eol_flags(eol) | std::regex_constants::match_not_null);
  }

  ice::async<State> handle(std::smatch& sm, std::error_code& ec) const
  {
    ec.clear();
    co_return co_await handler_(sm, ec);
  }

private:
  std::regex regex_;
  std::function<ice::async<State>(std::smatch&, std::error_code&)> handler_;
};

template <typename State>
class regex_state_machine {
public:
  regex_state_machine(State state) : state_(state) {}

  template <typename Handler>
  void add(State state, const std::string& regex, Handler&& handler)
  {
    add(state, regex, false, std::forward<Handler>(handler));
  }

  template <typename Handler>
  void add(State state, const std::string& regex, bool icase, Handler&& handler)
  {
    handlers_[state].emplace_back(regex, icase, std::forward<Handler>(handler));
  }

  ice::async<bool> handle(const std::string& string, bool eol, std::error_code& ec) noexcept
  {
    ec.clear();
    const auto handlers = handlers_.find(state_);
    if (handlers == handlers_.end()) {
      ice::log::warning("state without entry: {}", static_cast<int>(state_));
      return false;
    }
    if (handlers->second.empty()) {
      ice::log::warning("state without handlers: {}", static_cast<int>(state_));
      return false;
    }
    for (const auto& e : handlers->second) {
      std::smatch sm;
      if (e.match(string, sm, eol)) {
        state_ = co_await e.handle(sm, ec);
        co_return true;
      }
    }
    co_return false;
  }

  State state() const noexcept
  {
    return state_;
  }

  void state(State state) noexcept
  {
    state_ = state;
  }

private:
  std::map<State, std::vector<handler<State>>> handlers_;
  State state_;
};

class config {
public:
  enum class state {
    boot,
    login,
    done,
  };

  using async_state = ice::async<state>;

  config(ice::net::service& service) noexcept : port_(service)
  {
    // Cisco BootLoader Version : 8.5.103.0 (Cisco build) (Build time: Jul 25 2017 - 07:47:10)
    rsm_.add(state::boot, ".*BootLoader Version :\\s*([0-9\\.]*).*", [&](auto& sm, auto& ec) -> async_state {
      version_.bootloader = sm[1].str();
      co_return state::boot;
    });

    // Loading primary image (8.5.105.0)
    rsm_.add(state::boot, ".*Loading primary image \\(([^\\)]*).*", [&](auto& sm, auto& ec) -> async_state {
      version_.primary_image = sm[1].str();
      co_return state::boot;
    });

    // User:
    rsm_.add(state::boot, "\\s*User\\s*:\\s*", [&](auto& sm, auto& ec) -> async_state {
      ec = co_await exec("admin");  // 'admin' or 'Recover-Config'
      co_return state::login;
    });
  }

  ice::async<std::error_code> exec(std::string_view command = {}) noexcept
  {
    std::error_code ec;
    if (!command.empty()) {
      co_await ice::send(port_, command, ec);
      if (ec) {
        co_return ec;
      }
    }
    co_await ice::send(port_, "\r\n", ec);
    co_return ec;
  }

  ice::async<std::error_code> run(unsigned index = 0) noexcept
  {
    if (const auto ec = port_.open(index)) {
      ice::log::error(ec, "could not connect to serial port");
      co_return ec;
    }

    co_await exec();
    //co_await exec();

    std::string line;
    bool skip = false;
    std::string buffer;
    buffer.resize(128);

    while (rsm_.state() != state::done) {
      // Read data from serial port.
      std::error_code ec;
      const auto size = co_await port_.recv(buffer.data(), buffer.size(), ec);
      if (ec) {
        ice::log::error(ec, "serial port read error");
        co_return ec;
      }

      // Append data to line.
      for (auto c : std::string_view{ buffer.data(), size }) {
        if (skip) {
          if (c == '\n') {
            skip = false;
          }
        } else if (c != '\r') {
          line.push_back(c);
        }
      }

      // Search for EOL and handle line.
      for (std::size_t i = 0, max = line.size(); i < max; i++) {
        while (line[i] == '\n') {
          const std::string s{ line.data(), i };
          if (co_await rsm_.handle(s, true, ec)) {
            ice::log::info(s);
            if (ec) {
              ice::log::error(ec, "handle error");
              co_return ec;
            }
          } else {
            ice::log::debug(s);
          }
          line.erase(0, i + 1);
          max = line.size();
          i = 0;
        }
      }

      // Resume on timeout or empty line.
      if (size || line.empty()) {
        continue;
      }

      // Try to handle line on timeout.
      if (co_await rsm_.handle(line, false, ec)) {
        ice::log::info(line);
        if (ec) {
          ice::log::error(ec, "handle error");
          co_return ec;
        }
        skip = true;
      }
    }
    co_return{};
  }

private:
  struct version {
    std::string bootloader;
    std::string primary_image;
  } version_;

  ice::net::serial::port port_;
  regex_state_machine<state> rsm_{ state::boot };
};

int main()
{
  ice::net::service service;
  if (const auto ec = service.create()) {
    return ice::log::error(ec, "could not create service");
  }

  [&]() noexcept->ice::task
  {
    const auto ose = ice::on_scope_exit([&]() { service.stop(); });
    config config{ service };
    co_await config.run();
    ice::log::info("done");
  }
  ();

  if (const auto ec = service.run()) {
    return ice::log::notice(ec, "service run error");
  }
}

#if 0
#  include <ice/net/endpoint.hpp>
#  include <ice/net/service.hpp>
#  include <ice/net/ssh/session.hpp>
#  include <ice/utility.hpp>

ice::log::task client(ice::net::service& service, ice::net::endpoint endpoint) noexcept
{
  const auto ose = ice::on_scope_exit([&]() { service.stop(); });
  ice::net::ssh::session session{ service };
  if (const auto ec = session.create(endpoint.family())) {
    ice::log::error(ec, "could not create ssh session");
    co_return;
  }
  ice::log::info("connecting to {} ...", endpoint);
  if (const auto ec = co_await session.connect(endpoint)) {
    ice::log::error(ec, "connection failed");
    co_return;
  }
  if (const auto ec = co_await session.authenticate("test", "test")) {
    ice::log::error(ec, "username/password authentication failed");
    co_return;
  }
  std::error_code ec;
  auto channel = co_await session.open(ec);
  if (!channel) {
    ice::log::error(ec, "could not open channel");
    co_return;
  }
  const auto code = co_await channel.exec("ls /", ec);
  if (ec) {
    ice::log::error(ec, "could not execute 'ls'");
    co_return;
  }
  ice::log::info("'ls' returned {}", code);
  std::string buffer;
  buffer.resize(1024);
  while (true) {
    const auto size = co_await channel.recv(stdout, buffer.data(), buffer.size(), ec);
    if (ec) {
      ice::log::error(ec, "recv: {}", size);
      co_return;
    }
    if (!size) {
      break;
    }
    ice::log::info(std::string{ buffer.data(), size });
  }
  ice::log::info("done");
  co_return;
}

int main()
{
  ice::net::endpoint endpoint;
  if (const auto ec = endpoint.create("10.11.94.180", 22)) {
    return ice::log::error(ec, "could not create endpoint");
  }
  ice::net::service service;
  if (const auto ec = service.create()) {
    return ice::log::error(ec, "could not create service");
  }
  client(service, endpoint);
  if (const auto ec = service.run()) {
    return ice::log::notice(ec, "service run error");
  }
}
#endif
