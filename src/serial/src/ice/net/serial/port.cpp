#include "ice/net/serial/port.hpp"
#include <ice/error.hpp>
#include <ice/log.hpp>
#include <ice/net/event.hpp>
#include <regex>
#include <string>

#if ICE_OS_WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#endif

namespace ice::net::serial {

void port::close_type::operator()(std::uintptr_t handle) noexcept
{
  ::CloseHandle(reinterpret_cast<HANDLE>(handle));
}

std::error_code port::open(unsigned index) noexcept
{
  if (!index) {
    std::string buffer;
    DWORD size = 0;
    DWORD code = 0;
    do {
      buffer.resize(buffer.size() + MAX_PATH);
      size = ::QueryDosDevice(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
      if (!size) {
        code = ::GetLastError();
      }
    } while (size == 0 && code == ERROR_INSUFFICIENT_BUFFER);
    if (size == 0) {
      return ice::make_error_code(code);
    }
    buffer.resize(size);
    std::regex re{ "COM(\\d+)", std::regex_constants::ECMAScript | std::regex_constants::optimize };
    for (std::size_t pos = 0; pos < buffer.size();) {
      const auto device = std::string{ buffer.data() + pos, std::strlen(buffer.data() + pos) };
      std::smatch sm;
      if (std::regex_match(device, sm, re)) {
        index = std::stoul(sm[1].str());
        break;
      }
      pos += device.size() + 1;
    }
  }

  ice::log::info("Connecting to COM{} ...", index);

  const auto name = std::string("\\\\.\\COM") + std::to_string(index);
  handle_type handle{ ::CreateFile(
    name.data(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr) };
  if (!handle) {
    return make_error_code(::GetLastError());
  }

  if (!::CreateIoCompletionPort(handle.as<HANDLE>(), service().handle().as<HANDLE>(), 0, 0)) {
    return make_error_code(::GetLastError());
  }
  if (!::SetFileCompletionNotificationModes(handle.as<HANDLE>(), FILE_SKIP_COMPLETION_PORT_ON_SUCCESS)) {
    return make_error_code(::GetLastError());
  }

  DCB config = {};
  config.DCBlength = sizeof(config);
  if (!::GetCommState(handle.as<HANDLE>(), &config)) {
    return make_error_code(::GetLastError());
  }
  config.BaudRate = CBR_9600;
  config.ByteSize = 8;
  config.StopBits = ONESTOPBIT;
  config.Parity = NOPARITY;
  if (!::SetCommState(handle.as<HANDLE>(), &config)) {
    return make_error_code(::GetLastError());
  }

  COMMTIMEOUTS timeouts = {};

  // The maximum time interval between arrival of two bytes.
  timeouts.ReadIntervalTimeout = 50;  // milliseconds

  // Time added to the product of ReadTotalTimeoutMultiplier and the requested number of bytes.
  timeouts.ReadTotalTimeoutConstant = 720;  // milliseconds

  // For each each read operation, this value is multiplied by the requested number of bytes to be read.
  timeouts.ReadTotalTimeoutMultiplier = 10;  // milliseconds

  // Similar to ReadTotalTimeoutConstant but for write operations.
  timeouts.WriteTotalTimeoutConstant = 100;  // milliseconds

  // Similar to ReadTotalTimeoutMultiplier but for write operation.
  timeouts.WriteTotalTimeoutMultiplier = 10;  // milliseconds

  if (!::SetCommTimeouts(handle.as<HANDLE>(), &timeouts)) {
    return make_error_code(::GetLastError());
  }

  handle_ = std::move(handle);
  return {};
}

async<std::size_t> port::recv(char* data, std::size_t size, std::error_code& ec) noexcept
{
  ec.clear();
  event ev;
  DWORD bytes = 0;
  const auto handle = handle_.as<HANDLE>();
  if (::ReadFile(handle, data, static_cast<DWORD>(size), &bytes, &ev)) {
    co_return bytes;
  }
  if (const auto rc = ::GetLastError(); rc != ERROR_IO_PENDING) {
    ec = make_error_code(rc);
    co_return{};
  }
  co_await ev;
  if (!::GetOverlappedResult(handle, &ev, &bytes, FALSE)) {
    ec = make_error_code(::GetLastError());
    co_return{};
  }
  co_return bytes;
}

async<std::size_t> port::send(const char* data, std::size_t size, std::error_code& ec) noexcept
{
  ec.clear();
  DWORD bytes = 0;
  std::size_t written = 0;
  const auto handle = handle_.as<HANDLE>();
  do {
    event ev;
    if (!::WriteFile(handle, data, static_cast<DWORD>(size), &bytes, &ev)) {
      if (const auto rc = ::GetLastError(); rc != ERROR_IO_PENDING) {
        ec = make_error_code(rc);
        break;
      }
      co_await ev;
      if (!::GetOverlappedResult(handle, &ev, &bytes, FALSE)) {
        ec = make_error_code(::GetLastError());
        break;
      }
    }
    if (bytes == 0) {
      break;
    }
    data += bytes;
    size -= bytes;
    written += bytes;
  } while (size > 0);
  co_return written;
}

}  // namespace ice::net::serial
