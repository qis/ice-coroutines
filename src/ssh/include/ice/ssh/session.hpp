#pragma once
#include <ice/async.hpp>
#include <ice/log.hpp>
#include <ice/net/endpoint.hpp>
#include <ice/net/service.hpp>
#include <ice/ssh/config.hpp>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>

typedef struct ssh_session_struct* ssh_session;

namespace ice::ssh {

enum class verbosity { none, warning, protocol, packet, functions };

class session_interface {
public:
  virtual ~session_interface() = default;

  virtual void set_user(const std::string& name) = 0;
  virtual void set_directory(const std::filesystem::path& directory) = 0;
  virtual void set_known_hosts(const std::filesystem::path& file) = 0;
  virtual void set_timeout(std::chrono::microseconds us) = 0;
  virtual void set_verbosity(ssh::verbosity verbosity) = 0;
  virtual void add_identity(const std::filesystem::path& file) = 0;

  virtual async<void> connect(net::endpoint endpoint) = 0;

  virtual net::service& service() = 0;
  virtual const net::service& service() const = 0;

  virtual ssh_session handle() noexcept = 0;
  virtual const ssh_session handle() const noexcept = 0;
};

ICE_SSH_EXPORT std::unique_ptr<session_interface> create_session(net::service& service);

}  // namespace ice::ssh
