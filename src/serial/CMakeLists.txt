option(BUILD_SERIAL "Build secure shell support" ON)
if(NOT BUILD_SERIAL)
  return()
endif()

file(GLOB_RECURSE headers CONFIGURE_DEPENDS include/ice/net/serial/*.hpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include/ice/net/serial PREFIX "include" FILES ${headers})

file(GLOB_RECURSE sources CONFIGURE_DEPENDS src/ice/net/serial/*.hpp src/ice/net/serial/*.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src/ice/net/serial PREFIX "src" FILES ${sources})

add_library(serial STATIC ${headers} ${sources})
set_target_properties(serial PROPERTIES OUTPUT_NAME "ice-serial")

target_include_directories(serial PRIVATE src PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

target_link_libraries(serial PUBLIC ice::ice)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_link_libraries(serial PUBLIC c++experimental)
endif()

install(TARGETS serial EXPORT serial
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ice DESTINATION include FILES_MATCHING PATTERN "*.hpp")
install(EXPORT serial FILE ice-serial-targets.cmake NAMESPACE ice:: DESTINATION lib/cmake/ice)

add_library(ice::serial ALIAS serial)
