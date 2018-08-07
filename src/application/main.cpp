#include "firmware.hpp"
#include "startup.hpp"
#include <ice/utility.hpp>
#include <string>
#include <cstdlib>

int main()
{
  ice::net::service service;
  if (const auto ec = service.create()) {
    return ice::log::error(ec, "could not create service");
  }

  const std::string ip = "10.11.201.16";
  const std::string nm = "255.255.255.192";
  const std::string gw = "10.11.201.1";
  const unsigned vlan = 201;

  ice::net::endpoint endpoint;
  if (const auto ec = endpoint.create(ip, 22)) {
    ice::log::error(ec, "could not create endpoint");
    return 1;
  }

  // clang-format off
#if 0

  [&]() noexcept->ice::task {
    const auto ose = ice::on_scope_exit([&]() { service.stop(); });

    //startup startup{ service, ip, nm, gw, vlan };
    //if (const auto ec = co_await startup.run()) {
    //  co_return;
    //}

    firmware firmware{ service };
    if (const auto ec = co_await firmware.run(endpoint)) {
      co_return;
    }
  }();

#endif

  [&]() noexcept->ice::task {
    const auto ose = ice::on_scope_exit([&]() { service.stop(); });

    ice::net::endpoint endpoint;
    if (const auto ec = endpoint.create("127.0.0.1", 9000)) {
      ice::log::error(ec, "could not create endpoint");
      co_return;
    }

    ice::net::tcp::socket socket{ service };
    if (const auto ec = socket.create(endpoint.family())) {
      ice::log::error(ec, "could not create socket");
      co_return;
    }

    if (const auto ec = socket.set(ice::net::option::recv_timeout{ std::chrono::milliseconds{ 500 } })) {
      ice::log::error(ec, "could not set recv timeout");
      co_return;
    }

    if (const auto ec = socket.set(ice::net::option::send_timeout{ std::chrono::milliseconds{ 500 } })) {
      ice::log::error(ec, "could not set send timeout");
      co_return;
    }

    if (const auto ec = co_await socket.connect(endpoint)) {
      ice::log::error(ec, "could not connect");
      co_return;
    }

    std::error_code ec;
    const auto send_size = co_await socket.send("test", 4, ec);
    if (ec) {
      ice::log::error(ec, "send error");
      co_return;
    }
    std::string buffer;
    buffer.resize(10);
    const auto recv_size = co_await socket.recv(buffer.data(), buffer.size(), ec);
    if (ec) {
      ice::log::error(ec, "recv error");
      co_return;
    }
    ice::log::info("recv: {}", std::string{ buffer.data(), recv_size });

  }();

  // clang-format on

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
