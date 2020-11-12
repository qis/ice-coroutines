option(BUILD_SSH "Build secure shell support" ON)
if(NOT BUILD_SSH)
  return()
endif()

file(GLOB_RECURSE headers CONFIGURE_DEPENDS include/ice/net/ssh/*.hpp)
file(GLOB_RECURSE sources CONFIGURE_DEPENDS src/ice/net/ssh/*.hpp src/ice/net/ssh/*.cpp)

add_library(ssh STATIC ${headers} ${sources})
set_target_properties(ssh PROPERTIES OUTPUT_NAME "ice-ssh")

target_include_directories(ssh PRIVATE src PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

target_link_libraries(ssh PUBLIC ice::ice)

find_package(Libssh2 CONFIG REQUIRED)
target_link_libraries(ssh PUBLIC Libssh2::libssh2)

install(TARGETS ssh EXPORT ssh
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ice DESTINATION include FILES_MATCHING PATTERN "*.hpp")
install(EXPORT ssh FILE ice-ssh-targets.cmake NAMESPACE ice:: DESTINATION lib/cmake/ice)

add_library(ice::ssh ALIAS ssh)
