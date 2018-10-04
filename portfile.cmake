if (VCPKG_LIBRARY_LINKAGE STREQUAL dynamic)
  message(STATUS "Warning: Dynamic building not supported. Building static.")
  set(VCPKG_LIBRARY_LINKAGE static)
endif()

include(vcpkg_common_functions)

set(BUILD_SSH OFF)
if("ssh" IN_LIST FEATURES)
  set(BUILD_SSH ON)
endif()

set(BUILD_SERIAL OFF)
if("serial" IN_LIST FEATURES)
  set(BUILD_SERIAL ON)
endif()

vcpkg_configure_cmake(SOURCE_PATH ${CURRENT_PORT_DIR} PREFER_NINJA OPTIONS
  -DBUILD_SERIAL=${BUILD_SERIAL}
  -DBUILD_SSH=${BUILD_SSH}
  -DENABLE_EXCEPTIONS=ON
  -DENABLE_RTTI=ON)

vcpkg_install_cmake()
vcpkg_fixup_cmake_targets(CONFIG_PATH lib/cmake/${PORT})

file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/include ${CURRENT_PACKAGES_DIR}/debug/share)
file(INSTALL ${CURRENT_PORT_DIR}/license.txt DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)

vcpkg_copy_pdbs()

file(COPY ${CMAKE_CURRENT_LIST_DIR}/res/usage DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT})
