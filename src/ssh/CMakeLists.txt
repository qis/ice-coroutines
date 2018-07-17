option(BUILD_SSH "Build secure shell support" ON)
if(NOT BUILD_SSH)
  return()
endif()

file(GLOB_RECURSE headers CONFIGURE_DEPENDS include/ice/ssh/*.hpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include/ice/ssh PREFIX "include" FILES ${headers})

file(GLOB_RECURSE sources CONFIGURE_DEPENDS src/ice/ssh/*.hpp src/ice/ssh/*.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src/ice/ssh PREFIX "src" FILES ${sources})

add_library(ssh STATIC ${headers} ${sources})
set_target_properties(ssh PROPERTIES OUTPUT_NAME "ice-ssh")

target_include_directories(ssh PRIVATE src PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

target_link_libraries(ssh PUBLIC ice::net)

#find_path(LIBSSH_ROOT include/libssh/libssh.h)
#add_library(libssh STATIC IMPORTED)
#set_target_properties(libssh PROPERTIES
#  INTERFACE_INCLUDE_DIRECTORIES ${LIBSSH_ROOT}/include
#  IMPORTED_LOCATION_DEBUG ${LIBSSH_ROOT}/debug/lib/${CMAKE_STATIC_LIBRARY_PREFIX}ssh${CMAKE_STATIC_LIBRARY_SUFFIX}
#  IMPORTED_LOCATION_RELEASE ${LIBSSH_ROOT}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}ssh${CMAKE_STATIC_LIBRARY_SUFFIX})
#target_link_libraries(ssh PRIVATE libssh)

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
