#include "ice/ssh/log.hpp"
#include <ice/ssh/error.hpp>
#include <libssh/callbacks.h>
#include <libssh/libssh.h>
#include <mutex>

namespace ice::ssh::log {
namespace {

std::mutex g_mutex;
std::function<void(ice::log::level level, const char* message)> g_handler;

}  // namespace

ICE_SSH_EXPORT ice::log::level level()
{
  switch (ssh_get_log_level()) {
  case SSH_LOG_WARN: return ice::log::level::warning;
  case SSH_LOG_INFO: return ice::log::level::info;
  case SSH_LOG_DEBUG: return ice::log::level::debug;
  case SSH_LOG_TRACE: return ice::log::level::debug;
  }
  return ice::log::level::error;
}

ICE_SSH_EXPORT void level(ice::log::level level)
{
  auto native_level = 0;
  switch (level) {
  case ice::log::level::emergency: [[fallthrough]];
  case ice::log::level::alert: [[fallthrough]];
  case ice::log::level::critical: [[fallthrough]];
  case ice::log::level::error: native_level = SSH_LOG_NONE; break;
  case ice::log::level::warning: native_level = SSH_LOG_WARN; break;
  case ice::log::level::notice: [[fallthrough]];
  case ice::log::level::info: native_level = SSH_LOG_INFO; break;
  case ice::log::level::debug: native_level = SSH_LOG_DEBUG; break;
  }
  if (ssh_set_log_level(native_level) < 0) {
    throw_exception(domain_error{ "could not set log level" });
  }
}

ICE_SSH_EXPORT void handler(std::function<void(ice::log::level level, const char* message)> handler)
{
  std::lock_guard lock{ g_mutex };
  g_handler = handler;
}

ICE_SSH_EXPORT void write(ice::log::level level, const char* message)
{
  std::lock_guard lock{ g_mutex };
  if (g_handler) {
    g_handler(level, message);
  }
}

}  // namespace ice::ssh::log
