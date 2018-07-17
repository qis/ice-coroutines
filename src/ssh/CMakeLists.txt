option(BUILD_SSH "Build secure shell support" ON)
if(NOT BUILD_SSH)
  return()
endif()

file(GLOB_RECURSE headers CONFIGURE_DEPENDS include/ice/ssh/*.hpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include/ice/ssh PREFIX "include" FILES ${headers})

file(GLOB_RECURSE sources CONFIGURE_DEPENDS src/ice/ssh/*.hpp src/ice/ssh/*.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src/ice/ssh PREFIX "src" FILES ${sources})

add_library(ssh SHARED ${headers} ${sources})
set_target_properties(ssh PROPERTIES OUTPUT_NAME "ice-ssh")

if(WIN32)
  configure_file(main.rc.in ${CMAKE_CURRENT_BINARY_DIR}/main.rc)
  source_group("res" FILES ${CMAKE_CURRENT_BINARY_DIR}/main.rc main.manifest)
  target_sources(ssh PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/main.rc main.manifest)
endif()

target_include_directories(ssh PRIVATE src PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

target_link_libraries(ssh PUBLIC ice::net)

find_path(ICE_LIBSSH_INCLUDE libssh/libssh.h)
if(NOT ICE_LIBSSH_INCLUDE)
  message(FATAL_ERROR "Missing header file: libssh/libssh.h")
endif()

find_library(ICE_LIBSSH_RELEASE NAMES ssh PATHS ${ICE_LIBSSH_INCLUDE}/../lib NO_DEFAULT_PATH)
if(NOT ICE_LIBSSH_RELEASE)
  message(FATAL_ERROR "Missing vcpkg library: ssh (release)")
endif()

find_library(ICE_LIBSSH_DEBUG NAMES ssh PATHS ${ICE_LIBSSH_INCLUDE}/../debug/lib NO_DEFAULT_PATH)
if(NOT ICE_LIBSSH_DEBUG)
  message(FATAL_ERROR "Missing vcpkg library: ssh (debug)")
endif()

target_include_directories(ssh PRIVATE ${ICE_LIBSSH_INCLUDE})
target_link_libraries(ssh PRIVATE
  $<$<NOT:$<CONFIG:DEBUG>>:${ICE_LIBSSH_RELEASE}>
  $<$<CONFIG:DEBUG>:${ICE_LIBSSH_DEBUG}>)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_link_libraries(ssh PUBLIC c++experimental)
endif()

install(TARGETS ssh EXPORT ssh
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ice/ssh DESTINATION include/ice FILES_MATCHING PATTERN "*.hpp")
install(EXPORT ssh FILE ice-ssh-targets.cmake NAMESPACE ice:: DESTINATION lib/cmake/ice)

add_library(ice::ssh ALIAS ssh)
