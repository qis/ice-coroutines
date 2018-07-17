#include "ice/ssh/error.hpp"
#include <libssh/libssh.h>
#include <libssh/server.h>
#include <string>

namespace ice::ssh {
namespace {

inline std::string get(ssh_session session)
{
  if (const auto info = ssh_get_error(session)) {
    return info;
  }
  return {};
}

inline std::string get(ssh_bind bind)
{
  if (const auto info = ssh_get_error(bind)) {
    return info;
  }
  return {};
}

}  // namespace

domain_error::domain_error(std::string what) : what_(std::move(what)) {}
domain_error::domain_error(std::string what, std::string info) : what_(std::move(what)), info_(std::move(info)) {}
domain_error::domain_error(std::string what, ssh_session session) : what_(std::move(what)), info_(get(session)) {}
domain_error::domain_error(std::string what, ssh_bind bind) : what_(std::move(what)), info_(get(bind)) {}

}  // namespace ice::ssh
