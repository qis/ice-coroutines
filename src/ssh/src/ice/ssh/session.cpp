#include "ice/ssh/session.hpp"
#include <ice/ssh/error.hpp>
#include <ice/ssh/log.hpp>
#include <libssh/callbacks.h>
#include <libssh/libssh.h>

namespace ice::ssh {
namespace {

struct library {
  library()
  {
    if (const auto rv = ssh_init()) {
      throw_exception(domain_error{ "could not to initialize global cryptographic data structures" });
    }
  }
};

void log_callback(ssh_session, int priority, const char* message, void*)
{
  auto level = ice::log::level::info;
  switch (priority) {
  case SSH_LOG_WARN: level = ice::log::level::warning; break;
  case SSH_LOG_DEBUG: [[fallthrough]];
  case SSH_LOG_TRACE: level = ice::log::level::debug; break;
  }
  log::write(level, message ? message : "");
}

}  // namespace

class session final : public session_interface {
public:
  session(net::service& service) : service_(service)
  {
    const static library library;
    handle_.reset(ssh_new());
    if (!handle_) {
      throw_exception(domain_error{ "could not to create a new ssh session" });
    }
    set_verbosity(ssh::verbosity::none);
    if (ssh_set_log_userdata(handle_.get()) < 0) {
      throw_exception(domain_error{ "could not set log user data", handle_.get() });
    }
    ssh_callbacks_struct cb = {};
    cb.log_function = log_callback;
    ssh_callbacks_init(&cb);
    if (ssh_set_callbacks(handle_.get(), &cb) < 0) {
      throw_exception(domain_error{ "could not set callbacks", handle_.get() });
    }
  }

  void set_user(const std::string& name) override
  {
    if (ssh_options_set(handle_.get(), SSH_OPTIONS_USER, name.data()) < 0) {
      throw_exception(domain_error{ "could not set user name", handle_.get() });
    }
  }

  void set_directory(const std::filesystem::path& directory) override
  {
    if (ssh_options_set(handle_.get(), SSH_OPTIONS_SSH_DIR, directory.u8string().data()) < 0) {
      throw_exception(domain_error{ "could not set ssh directory", handle_.get() });
    }
  }

  void set_known_hosts(const std::filesystem::path& file) override
  {
    if (ssh_options_set(handle_.get(), SSH_OPTIONS_KNOWNHOSTS, file.u8string().data()) < 0) {
      throw_exception(domain_error{ "could not set known hosts", handle_.get() });
    }
  }

  void set_timeout(std::chrono::microseconds us) override
  {
    const auto value = static_cast<long>(us.count());
    if (ssh_options_set(handle_.get(), SSH_OPTIONS_TIMEOUT_USEC, &value) < 0) {
      throw_exception(domain_error{ "could not set timeout", handle_.get() });
    }
  }

  void set_verbosity(ssh::verbosity verbosity) override
  {
    auto value = 0;
    switch (verbosity) {
    case ssh::verbosity::none: value = SSH_LOG_NOLOG; break;
    case ssh::verbosity::warning: value = SSH_LOG_WARNING; break;
    case ssh::verbosity::protocol: value = SSH_LOG_PROTOCOL; break;
    case ssh::verbosity::packet: value = SSH_LOG_PACKET; break;
    case ssh::verbosity::functions: value = SSH_LOG_FUNCTIONS; break;
    }
    if (ssh_options_set(handle_.get(), SSH_OPTIONS_LOG_VERBOSITY, &value) < 0) {
      throw_exception(domain_error{ "could not verbosity", handle_.get() });
    }
  }

  void add_identity(const std::filesystem::path& file) override
  {
    if (ssh_options_set(handle_.get(), SSH_OPTIONS_ADD_IDENTITY, file.u8string().data()) < 0) {
      throw_exception(domain_error{ "could not add identity", handle_.get() });
    }
  }

  async<void> connect(net::endpoint endpoint) override
  {
    //ssh_options_set(handle_.get(), SSH_OPTIONS_HOST, "localhost");
    //ssh_options_set(handle_.get(), SSH_OPTIONS_PORT, &port);
    co_return;
  }


  net::service& service() override
  {
    return service_;
  }

  const net::service& service() const override
  {
    return service_;
  }

  ssh_session handle() noexcept override
  {
    return handle_.get();
  }

  const ssh_session handle() const noexcept override
  {
    return handle_.get();
  }

private:
  net::service& service_;
  std::unique_ptr<ssh_session_struct, decltype(&ssh_free)> handle_ = { nullptr, ssh_free };
};

ICE_SSH_EXPORT std::unique_ptr<session_interface> create_session(net::service& service)
{
  return std::make_unique<session>(service);
}

}  // namespace ice::ssh
