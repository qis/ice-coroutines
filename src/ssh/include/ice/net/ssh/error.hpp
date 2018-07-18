#pragma once
#include <system_error>

namespace ice::net::ssh {

const std::error_category& domain_category() noexcept;

}  // namespace ice::net::ssh
