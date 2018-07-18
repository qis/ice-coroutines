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
  if (const auto ec = co_await session.connect(endpoint)) {
    ice::log::error(ec, "could not connect to {}", endpoint);
    co_return;
  }
  ice::log::info("connecting to {} ...", endpoint);
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
  service.run();
}
