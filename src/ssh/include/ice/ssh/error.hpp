#pragma once
#include <ice/error.hpp>
#include <ice/ssh/config.hpp>
#include <ice/ssh/server.hpp>
#include <ice/ssh/session.hpp>
#include <exception>

namespace ice::ssh {

class domain_error final : public std::exception {
public:
  ICE_SSH_EXPORT domain_error(std::string what);
  ICE_SSH_EXPORT domain_error(std::string what, std::string info);
  ICE_SSH_EXPORT domain_error(std::string what, ssh_session session);
  ICE_SSH_EXPORT domain_error(std::string what, ssh_bind bind);

  const char* what() const noexcept override
  {
    return what_.data();
  }

  const char* info() const noexcept
  {
    return info_.data();
  }

private:
  const std::string what_;
  const std::string info_;
};

}  // namespace ice::ssh
