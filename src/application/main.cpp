#include <ice/async.hpp>
#include <ice/log.hpp>
#include <ice/net/tcp/socket.hpp>
#include <ice/service.hpp>
#include <ice/utility.hpp>

ice::task handle(ice::service& service, ice::net::tcp::socket client) noexcept
{
  std::string buffer;
  buffer.resize(1024);
  while (true) {
    const auto size = co_await client.recv(buffer.data(), buffer.size());
    if (size == 0) {
      service.stop();
      co_return;
    }
    ice::log::info("recv: {} ({})", std::string_view{ buffer.data(), size }, size);
  }
}

ice::task server(ice::service& service, ice::net::endpoint endpoint) noexcept
{
  ice::net::tcp::socket server{ service };
  if (const auto ec = server.create(endpoint.family())) {
    ice::log::error(ec, "could not create server socket");
    co_return;
  }
  if (const auto ec = server.set(ice::net::option::reuse_address{ true })) {
    ice::log::error(ec, "could not set server socket option: reuse address");
    co_return;
  }
  if (const auto ec = server.bind(endpoint)) {
    ice::log::error(ec, "could not bind server socket");
    co_return;
  }
  if (const auto ec = server.listen()) {
    ice::log::error(ec, "could not listen on server socket");
    co_return;
  }
  ice::log::notice("bind: {}", endpoint);
  while (true) {
    auto client = co_await server.accept(endpoint);
    ice::log::notice("accept: {}", endpoint);
    handle(service, std::move(client));
  }
  co_return;
}

ice::task client(ice::service& service, ice::net::endpoint endpoint) noexcept
{
  ice::net::tcp::socket client{ service };
  if (const auto ec = client.create(endpoint.family())) {
    ice::log::error(ec, "could not create client socket");
    co_return;
  }
  if (const auto ec = co_await client.connect(endpoint)) {
    ice::log::error(ec, "could not connect to server socket");
    co_return;
  }
  ice::log::notice("connect: {} -> {}", client.name(), endpoint);
  std::string message = "test";
  const auto size = co_await client.send(message.data(), message.size());
  ice::log::info("send: {} ({})", message, size);
}

int main()
{
  ice::net::endpoint endpoint;
  if (const auto ec = endpoint.create("127.0.0.1", 9000)) {
    return ice::log::error(ec, "could not create endpoint");
  }
  ice::service service;
  if (const auto ec = service.create()) {
    return ice::log::error(ec, "could not create service");
  }
  server(service, endpoint);
  client(service, endpoint);
  service.run();
}
