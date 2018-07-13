#include "utility.hpp"
#include <ice/error.hpp>
#include <fmt/ostream.h>
#include <thread>
#include <cassert>

#if ICE_OS_WIN32
#  include <windows.h>
#else
#  include <pthread.h>
#  if ICE_OS_FREEBSD
#    include <pthread_np.h>
#  endif
#endif

namespace ice {

void thread_local_storage::close_type::operator()(std::uintptr_t handle) noexcept
{
#if ICE_OS_WIN32
  ::TlsFree(static_cast<DWORD>(handle));
#else
  ::pthread_key_delete(static_cast<pthread_key_t>(handle));
#endif
}

thread_local_storage::lock::lock(handle_view handle, void* value) noexcept : handle_(handle)
{
#if ICE_OS_WIN32
  [[maybe_unused]] const auto rc = ::TlsSetValue(handle_.as<DWORD>(), value);
  assert(rc);
#else
  [[maybe_unused]] const auto rc = ::pthread_setspecific(handle_.as<pthread_key_t>(), value);
  assert(rc == 0);
#endif
}

thread_local_storage::lock::~lock()
{
#if ICE_OS_WIN32
  [[maybe_unused]] const auto rc = ::TlsSetValue(handle_.as<DWORD>(), nullptr);
  assert(rc);
#else
  [[maybe_unused]] const auto rc = ::pthread_setspecific(handle_.as<pthread_key_t>(), nullptr);
  assert(rc == 0);
#endif
}

thread_local_storage::thread_local_storage() noexcept
{
#if ICE_OS_WIN32
  handle_.reset(::TlsAlloc());
#else
  auto handle = ice::handle_view<pthread_key_t, -1>::invalid_value();
  [[maybe_unused]] const auto rc = ::pthread_key_create(&handle, nullptr);
  handle_.reset(handle);
  assert(rc == 0);
#endif
  assert(handle_);
}

void* thread_local_storage::get() noexcept
{
#if ICE_OS_WIN32
  return ::TlsGetValue(handle_.as<DWORD>());
#else
  return ::pthread_getspecific(handle_.as<pthread_key_t>());
#endif
}

const void* thread_local_storage::get() const noexcept
{
#if ICE_OS_WIN32
  return ::TlsGetValue(handle_.as<DWORD>());
#else
  return ::pthread_getspecific(handle_.as<pthread_key_t>());
#endif
}

std::error_code set_thread_affinity(std::size_t index) noexcept
{
  assert(index < std::thread::hardware_concurrency());
#if ICE_OS_WIN32
  if (!::SetThreadAffinityMask(::GetCurrentThread(), DWORD_PTR(1) << index)) {
    return make_error_code(::GetLastError());
  }
#else
#  if ICE_OS_LINUX
  cpu_set_t cpuset;
#  elif ICE_OS_FREEBSD
  cpuset_t cpuset;
#  endif
  CPU_ZERO(&cpuset);
  CPU_SET(static_cast<int>(index), &cpuset);
  if (const auto rc = ::pthread_setaffinity_np(::pthread_self(), sizeof(cpuset), &cpuset)) {
    return make_error_code(rc);
  }
#endif
  return {};
}

}  // namespace ice
