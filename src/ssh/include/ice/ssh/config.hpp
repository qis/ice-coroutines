#pragma once
#include <ice/config.hpp>

#ifndef ICE_SSH_EXPORT
#  ifdef _MSC_VER
#    ifdef ssh_EXPORTS
#      define ICE_SSH_EXPORT __declspec(dllexport)
#    else
#      define ICE_SSH_EXPORT __declspec(dllimport)
#    endif
#  else
#    define ICE_SSH_EXPORT
#  endif
#endif

#ifndef ICE_SSH_TEMPLATE
#  ifdef _MSC_VER
#    ifdef ssh_EXPORTS
#      define ICE_SSH_TEMPLATE
#    else
#      define ICE_SSH_TEMPLATE extern
#    endif
#  else
#    define ICE_SSH_TEMPLATE
#  endif
#endif
