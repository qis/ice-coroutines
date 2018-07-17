option(BUILD_NET "Build networking support" ON)
if(NOT BUILD_NET)
  return()
endif()

file(GLOB_RECURSE headers CONFIGURE_DEPENDS include/ice/net/*.hpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include/ice/net PREFIX "include" FILES ${headers})

file(GLOB_RECURSE sources CONFIGURE_DEPENDS src/ice/net/*.hpp src/ice/net/*.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src/ice/net PREFIX "src" FILES ${sources})

add_library(net STATIC ${headers} ${sources})
set_target_properties(net PROPERTIES OUTPUT_NAME "ice-net")

include(ConfigureHeader)
configure_header(config.hpp.in config.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/include/ice/net/config.hpp)
source_group("include" FILES ${CMAKE_CURRENT_BINARY_DIR}/include/ice/net/config.hpp)
target_sources(net PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include/ice/net/config.hpp)

target_include_directories(net PRIVATE src PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

target_link_libraries(net PUBLIC ice::ice)

find_package(OpenSSL REQUIRED)
target_link_libraries(net PUBLIC OpenSSL::SSL OpenSSL::Crypto)

if(WIN32)
  target_link_libraries(net PUBLIC ws2_32 mswsock)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_link_libraries(net PUBLIC c++experimental)
endif()

install(TARGETS net EXPORT net
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/ice/net/config.hpp DESTINATION include/ice/net)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ice/net DESTINATION include/ice FILES_MATCHING PATTERN "*.hpp")
install(EXPORT net FILE ice-net-targets.cmake NAMESPACE ice:: DESTINATION lib/cmake/ice)

add_library(ice::net ALIAS net)
