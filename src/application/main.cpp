#include <ice/async.hpp>
#include <ice/error.hpp>
#include <ice/log.hpp>
#include <ice/net/ssh/session.hpp>
#include <ice/utility.hpp>

ice::task test(ice::net::service& service)
{
  const std::string host = xorstr("127.0.0.1");
  const std::string pass = xorstr("test");

  const auto ose = ice::on_scope_exit([&]() { service.stop(); });

  ice::net::endpoint endpoint;
  if (const auto ec = endpoint.create(host, 22)) {
    ice::log::error(ec, "create reset endpoint");
    co_return;
  }

  ice::net::ssh::session session{ service };
  if (const auto ec = co_await session.connect(endpoint)) {
    ice::log::error(ec, "connect session");
    co_return;
  }
  if (const auto ec = co_await session.authenticate("admin", pass)) {
    ice::log::error(ec, "connect session");
    co_return;
  }
  if (const auto ec = co_await session.request_pty("vanilla")) {
    ice::log::error(ec, "request pty");
    co_return;
  }
  if (const auto ec = co_await session.open_shell()) {
    ice::log::error(ec, "open shell");
    co_return;
  }

  std::string buffer;
  buffer.resize(1024);
  while (true) {
    std::error_code ec;
    const auto data = co_await ice::recv(session, buffer, ec);
    std::fwrite(data.data(), data.size(), sizeof(char), stdout);
    std::fflush(stdout);
    if (ec) {
      std::puts("");
      ice::log::error(ec, "recv");
      break;
    }
  }
}

int main()
{
  ice::net::service service;
  if (const auto ec = service.create()) {
    return ice::log::error(ec, "create service");
  }

  test(service);

  if (const auto ec = service.run()) {
    return ice::log::error(ec, "run service");
  }
}
