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
  handler(const std::string& regex, Handler&& handler, bool icase = false) :
    regex_(regex, std::regex_constants::ECMAScript | default_icase_flags(icase) | std::regex_constants::optimize),
    handler_(std::forward<Handler>(handler))
  {}

  bool match(const std::string& string, std::smatch& sm, bool eol = true) const noexcept
  {
    return std::regex_match(string, sm, regex_, default_eol_flags(eol) | std::regex_constants::match_not_null);
  }

  ice::async<State> handle(std::smatch& sm, std::error_code& ec) const
  {
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
  void add(State state, const std::string& regex, Handler&& handler, bool icase = false)
  {
    handlers_[state].emplace_back(regex, std::forward<Handler>(handler), icase);
  }

  ice::async<std::error_code> handle(const std::string& string, bool eol = true) noexcept
  {
    const auto handlers = handlers_.find(state_);
    if (handlers == handlers_.end()) {
      ice::log::warning("state without entry: {}", static_cast<int>(state_));
      return {};
    }
    if (handlers->second.empty()) {
      ice::log::warning("state without handlers: {}", static_cast<int>(state_));
      return {};
    }
    for (const auto& e : handlers->second) {
      std::smatch sm;
      if (e.match(string, sm, eol)) {
        std::error_code ec;
        state_ = co_await e.handle(sm, ec);
        return ec;
      }
    }
    return {};
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
  };

  config(ice::net::service& service) noexcept : port_(service)
  {
    // Cisco BootLoader Version : 8.5.103.0 (Cisco build) (Build time: Jul 25 2017 - 07:47:10)
    rsm_.add(
      state::boot, ".*BootLoader Version :\\s*([0-9\\.]*).*",
      [&](std::smatch& sm, std::error_code& ec) -> ice::async<state> {
        ice::log::notice("boot loader version: {}", sm[1].str());
        // TODO: Save version string.
        co_return state::boot;
      },
      true);

    // Loading primary image (8.5.105.0)
    rsm_.add(
      state::boot, ".*Loading primary image \\(([^\\)]*).*",
      [&](std::smatch& sm, std::error_code& ec) -> ice::async<state> {
        ice::log::notice("primary image version: {}", sm[1].str());
        // TODO: Save version string.
        co_return state::boot;
      },
      true);

    // User:
    rsm_.add(
      state::boot, "\\s*User\\s*:\\s*",
      [&](std::smatch& sm, std::error_code& ec) -> ice::async<state> {
        ice::log::notice("User:");
        // TODO: Perform login operation.
        co_return state::login;
      },
      true);
  }

  ice::async<std::error_code> run(unsigned index) noexcept
  {
    if (const auto ec = port_.open(index)) {
      ice::log::error(ec, "could not open serial port: COM{}", index);
      co_return ec;
    }

    std::string buffer;
    buffer.resize(128);
    std::string line;
    while (true) {
      // Read data from serial port.
      std::error_code ec;
      const auto size = co_await port_.read(buffer.data(), buffer.size(), ec);
      if (ec) {
        ice::log::error(ec, "serial port read error");
        co_return ec;
      }

      // Append data to line.
      for (auto c : std::string_view{ buffer.data(), size }) {
        if (c != '\r') {
          line.push_back(c);
        }
      }

      // Search for EOL and handle line.
      for (std::size_t i = 0, max = line.size(); i < max; i++) {
        while (line[i] == '\n') {
          ec = co_await rsm_.handle(std::string(line.data(), i), true);
          if (ec) {
            if (ec != make_error_code(ice::errc::eof)) {
              ice::log::error(ec, "handle error");
            }
            co_return ec;
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
      ec = co_await rsm_.handle(line, false);
      if (ec) {
        if (ec == make_error_code(ice::errc::eof)) {
          co_return ec;
        }
        continue;
      }
    }
    co_return{};
  }

private:
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
    co_await config.run(3);
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
