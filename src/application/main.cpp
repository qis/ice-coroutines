#include <ice/async.hpp>
#include <ice/log.hpp>
#include <ice/net/tcp/socket.hpp>
#include <ice/utility.hpp>

ice::task server(ice::service& service) noexcept
{
  const auto ose = ice::on_scope_exit([&]() noexcept {
    service.stop();
  });
  ice::net::endpoint endpoint{ "127.0.0.1", 9001 };
  ice::net::tcp::socket server{ service };
  if (const auto ec = server.create(endpoint.family())) {
    ice::log::error(ec, "could not create socket");
    co_return;
  }
  if (const auto ec = server.set(ice::net::option::reuse_address{ true })) {
    ice::log::error(ec, "could not set socket reuse address option");
    co_return;
  }
  if (const auto ec = server.bind(endpoint)) {
    ice::log::error(ec, "could not bind to {}:{}", endpoint.host(), endpoint.port());
    co_return;
  }
  if (const auto ec = server.listen()) {
    ice::log::error(ec, "could not listen on {}:{}", endpoint.host(), endpoint.port());
    co_return;
  }
  auto client = co_await server.accept();
  ice::log::notice("accepted: {}:{}", client.remote_endpoint().host(), client.remote_endpoint().port());
}

int main()
{
  ice::service service;
  if (const auto ec = service.create()) {
    ice::log::error(ec, "could not create service");
    return 1;
  }
  server(service);
  service.run();
}
