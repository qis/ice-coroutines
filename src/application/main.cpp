#include <ice/log.hpp>
#include <ice/net/endpoint.hpp>
#include <ice/net/service.hpp>
#include <ice/net/ssh/session.hpp>
#include <ice/utility.hpp>

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
  auto channel = co_await session.open();
  if (!channel) {
    ice::log::error("could not open channel");
    co_return;
  }
  if (const auto ec = co_await channel.exec("/bin/ls")) {
    ice::log::error(ec, "could not execute 'ls'");
    co_return;
  }
  std::string buffer;
  buffer.resize(1024);
  std::error_code ec;
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
