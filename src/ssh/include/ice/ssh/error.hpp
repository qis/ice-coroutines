#pragma once
#include <ice/error.hpp>
#include <ice/ssh/config.hpp>
#include <ice/ssh/session.hpp>
#include <exception>

typedef struct ssh_bind_struct* ssh_bind;

namespace ice::ssh {

class domain_error : public std::exception {
public:
  domain_error(std::string what);
  domain_error(std::string what, std::string info);
  domain_error(std::string what, ssh_session session);
  domain_error(std::string what, ssh_bind bind);

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
