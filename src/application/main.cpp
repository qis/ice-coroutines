#include <ice/async.hpp>
#include <ice/log.hpp>
#include <ice/service.hpp>

int main()
{
  ice::service service;
  [&]() -> ice::task {
    co_await service.schedule(true);
    ice::log::notice("ok");
    service.stop();
  }();
  service.run();
}
