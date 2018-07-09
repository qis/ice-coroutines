#include <ice/async.hpp>
#include <ice/log.hpp>
#include <ice/net/tcp/socket.hpp>
#include <ice/utility.hpp>

ice::task server(ice::service& service)
{
  const auto ose = ice::on_scope_exit([&]() noexcept {
    service.stop();
  });
  ice::net::endpoint endpoint{ "127.0.0.1", 9001 };
  ice::log::notice(ice::color::red | ice::style::bold, "listening on {}:{}", endpoint.host(), endpoint.port());
  co_return;
}

int main()
{
  ice::service service;
  server(service);
  service.run();
}
