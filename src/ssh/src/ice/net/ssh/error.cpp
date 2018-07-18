#include "ice/net/ssh/error.hpp"
#include <libssh2.h>
#include <string>

namespace ice::net::ssh {
namespace {

class domain_category : public std::error_category {
public:
  const char* name() const noexcept override
  {
    return "ssh";
  }

  std::string message(int code) const override
  {
    switch (code) {
    case LIBSSH2_ERROR_SOCKET_NONE: return "no socket";
    case LIBSSH2_ERROR_BANNER_RECV: return "recv banner";
    case LIBSSH2_ERROR_BANNER_SEND: return "send banner";
    case LIBSSH2_ERROR_INVALID_MAC: return "invalid_mac";
    case LIBSSH2_ERROR_KEX_FAILURE: return "kex_failure";
    case LIBSSH2_ERROR_ALLOC: return "bad alloc";
    case LIBSSH2_ERROR_SOCKET_SEND: return "socket send error";
    case LIBSSH2_ERROR_KEY_EXCHANGE_FAILURE: return "key exchange error";
    case LIBSSH2_ERROR_TIMEOUT: return "timeout";
    case LIBSSH2_ERROR_HOSTKEY_INIT: return "hostkey init error";
    case LIBSSH2_ERROR_HOSTKEY_SIGN: return "hostkey sign error";
    case LIBSSH2_ERROR_DECRYPT: return "decrypt error";
    case LIBSSH2_ERROR_SOCKET_DISCONNECT: return "socket disconnected";
    case LIBSSH2_ERROR_PROTO: return "protocol error";
    case LIBSSH2_ERROR_PASSWORD_EXPIRED: return "password expired";
    case LIBSSH2_ERROR_FILE: return "file error";
    case LIBSSH2_ERROR_METHOD_NONE: return "missing method";
    case LIBSSH2_ERROR_AUTHENTICATION_FAILED: return "authentication failed";
#if LIBSSH2_ERROR_PUBLICKEY_UNRECOGNIZED != LIBSSH2_ERROR_AUTHENTICATION_FAILED
    case LIBSSH2_ERROR_PUBLICKEY_UNRECOGNIZED: return "publickey unrecognized";
#endif
    case LIBSSH2_ERROR_PUBLICKEY_UNVERIFIED: return "publickey unverified";
    case LIBSSH2_ERROR_CHANNEL_OUTOFORDER: return "channel out of order";
    case LIBSSH2_ERROR_CHANNEL_FAILURE: return "channel error";
    case LIBSSH2_ERROR_CHANNEL_REQUEST_DENIED: return "channel request denied";
    case LIBSSH2_ERROR_CHANNEL_UNKNOWN: return "channel unknown";
    case LIBSSH2_ERROR_CHANNEL_WINDOW_EXCEEDED: return "channel window exceeded";
    case LIBSSH2_ERROR_CHANNEL_PACKET_EXCEEDED: return "channel packet exceeded";
    case LIBSSH2_ERROR_CHANNEL_CLOSED: return "channel closed";
    case LIBSSH2_ERROR_CHANNEL_EOF_SENT: return "channel eof sent";
    case LIBSSH2_ERROR_SCP_PROTOCOL: return "scp protocol error";
    case LIBSSH2_ERROR_ZLIB: return "zlib error";
    case LIBSSH2_ERROR_SOCKET_TIMEOUT: return "socket timeout";
    case LIBSSH2_ERROR_SFTP_PROTOCOL: return "sftp protocol error";
    case LIBSSH2_ERROR_REQUEST_DENIED: return "request denied";
    case LIBSSH2_ERROR_METHOD_NOT_SUPPORTED: return "method not supported";
    case LIBSSH2_ERROR_INVAL: return "invalid";
    case LIBSSH2_ERROR_INVALID_POLL_TYPE: return "invalid poll type";
    case LIBSSH2_ERROR_PUBLICKEY_PROTOCOL: return "publickey protocol error";
    case LIBSSH2_ERROR_EAGAIN: return "eagain";
    case LIBSSH2_ERROR_BUFFER_TOO_SMALL: return "buffer too small";
    case LIBSSH2_ERROR_BAD_USE: return "bad use";
    case LIBSSH2_ERROR_COMPRESS: return "compress error";
    case LIBSSH2_ERROR_OUT_OF_BOUNDARY: return "out of boundary";
    case LIBSSH2_ERROR_AGENT_PROTOCOL: return "agent protocol";
    case LIBSSH2_ERROR_SOCKET_RECV: return "socket recv error";
    case LIBSSH2_ERROR_ENCRYPT: return "encrypt error";
    case LIBSSH2_ERROR_BAD_SOCKET: return "bad socket";
    case LIBSSH2_ERROR_KNOWN_HOSTS: return "known hosts";
    }
    return "error " + std::to_string(code);
  }
};

domain_category g_domain_category;

}  // namespace

const std::error_category& domain_category() noexcept
{
  return g_domain_category;
}

}  // namespace ice::net::ssh
