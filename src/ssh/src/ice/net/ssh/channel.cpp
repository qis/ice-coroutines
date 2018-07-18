#include "ice/net/ssh/channel.hpp"
#include <ice/net/ssh/error.hpp>

namespace ice::net::ssh {

void channel::close_type::operator()(LIBSSH2_CHANNEL* handle) noexcept
{

}

}  // namespace ice::net::ssh
