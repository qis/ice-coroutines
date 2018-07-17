#pragma once

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
