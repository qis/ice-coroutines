#pragma once
#include <ice/net/service.hpp>
#include <ice/ssh/config.hpp>
#include <memory>
#include <type_traits>

typedef struct ssh_session_struct* ssh_session;

namespace ice::ssh {

class session {
public:
  session(net::service& service);

  session(session&& other) = default;
  session& operator=(session&& other) = default;

  ~session();

  ssh_session handle() noexcept
  {
    return handle_.get();
  }

  const ssh_session handle() const noexcept
  {
    return handle_.get();
  }

private:
  std::reference_wrapper<net::service> service_;
  std::unique_ptr<ssh_session_struct, void(*)(ssh_session)> handle_;
};

}  // namespace ice::ssh
