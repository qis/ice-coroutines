#pragma once
#include <ice/log.hpp>
#include <ice/ssh/config.hpp>
#include <functional>

namespace ice::ssh::log {

ICE_SSH_EXPORT ice::log::level level();
ICE_SSH_EXPORT void level(ice::log::level level);
ICE_SSH_EXPORT void handler(std::function<void(ice::log::level level, const char* message)> handler);
ICE_SSH_EXPORT void write(ice::log::level level, const char* message);

}  // namespace ice::ssh::log
