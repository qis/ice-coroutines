option(BUILD_SERIAL "Build secure shell support" ON)
if(NOT BUILD_SERIAL)
  return()
endif()

file(GLOB_RECURSE headers CONFIGURE_DEPENDS include/ice/net/*.hpp)
file(GLOB_RECURSE sources CONFIGURE_DEPENDS src/ice/net/*.hpp src/ice/net/*.cpp)

add_library(serial STATIC ${headers} ${sources})
set_target_properties(serial PROPERTIES OUTPUT_NAME "ice-serial")

target_include_directories(serial PRIVATE src PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

target_link_libraries(serial PUBLIC ice::ice)

install(TARGETS serial EXPORT serial
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ice DESTINATION include FILES_MATCHING PATTERN "*.hpp")
install(EXPORT serial FILE ice-serial-targets.cmake NAMESPACE ice:: DESTINATION lib/cmake/ice)

add_library(ice::serial ALIAS serial)
