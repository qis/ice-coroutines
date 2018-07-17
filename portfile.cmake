if (VCPKG_LIBRARY_LINKAGE STREQUAL static)
  message(STATUS "Warning: Static building not supported. Building dynamic.")
  set(VCPKG_LIBRARY_LINKAGE dynamic)
endif()

include(vcpkg_common_functions)

set(BUILD_NET OFF)
if("net" IN_LIST FEATURES)
  set(BUILD_NET ON)
endif()

set(BUILD_SSH OFF)
if("ssh" IN_LIST FEATURES)
  set(BUILD_NET ON)
  set(BUILD_SSH ON)
endif()

vcpkg_configure_cmake(SOURCE_PATH ${CURRENT_PORT_DIR} PREFER_NINJA OPTIONS
  -DBUILD_NET=${BUILD_NET} -DBUILD_SSH=${BUILD_SSH})

vcpkg_install_cmake()
vcpkg_fixup_cmake_targets(CONFIG_PATH lib/cmake/${PORT})

file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/include ${CURRENT_PACKAGES_DIR}/debug/share)
file(INSTALL ${CURRENT_PORT_DIR}/license.txt DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)

vcpkg_copy_pdbs()

file(COPY ${CMAKE_CURRENT_LIST_DIR}/res/usage DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT})
