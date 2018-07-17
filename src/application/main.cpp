#include <ice/log.hpp>
#include <ice/net/endpoint.hpp>
#include <ice/net/service.hpp>
#include <ice/ssh/log.hpp>
#include <ice/ssh/session.hpp>
#include <ice/utility.hpp>

ice::log::task client(ice::net::service& service, ice::net::endpoint endpoint) noexcept
{
  const auto ose = ice::on_scope_exit([&]() { service.stop(); });
  ice::ssh::log::level(ice::log::level::debug);
  ice::ssh::log::handler([](auto level, auto message) {
    ice::log::queue(ice::log::clock::now(), level, message);
  });
  auto session = ice::ssh::create_session(service);
  session->set_timeout(std::chrono::seconds{ 3 });
  co_await session->connect(endpoint);
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
