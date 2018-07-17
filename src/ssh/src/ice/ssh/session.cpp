#include "ice/ssh/session.hpp"
#include <ice/ssh/error.hpp>
#include <libssh/libssh.h>

namespace ice::ssh {
namespace {

void session_deleter(ssh_session session)
{
  if (session) {
    ssh_free(session);
  }
}

struct library {
  library()
  {
    if (const auto rv = ssh_init()) {
      throw_exception(domain_error{ "failed to initialize global cryptographic data structures" });
    }
  }
};

}  // namespace

session::session(net::service& service) : service_(service), handle_(nullptr, session_deleter)
{
  const static library library;
  handle_.reset(ssh_new());
  if (!handle_) {
    throw_exception(domain_error{ "failed to create a new ssh session" });
  }
}

session::~session() {}


}  // namespace ice::ssh
