file(GLOB_RECURSE headers CONFIGURE_DEPENDS include/ice/*.hpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include/ice PREFIX "include" FILES ${headers})

file(GLOB_RECURSE sources CONFIGURE_DEPENDS src/ice/*.hpp src/ice/*.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src/ice PREFIX "src" FILES ${sources})

add_library(ice STATIC ${headers} ${sources})

include(ConfigureHeader)
configure_header(config.hpp.in config.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/include/ice/config.hpp)
source_group("include" FILES ${CMAKE_CURRENT_BINARY_DIR}/include/ice/config.hpp)
target_sources(ice PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include/ice/config.hpp)

target_include_directories(ice PRIVATE src PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

if(UNIX)
  find_package(Threads REQUIRED)
  target_link_libraries(ice PUBLIC Threads::Threads)
endif()

find_package(fmt CONFIG REQUIRED)
target_link_libraries(ice PUBLIC fmt::fmt)

find_package(OpenSSL REQUIRED)
target_link_libraries(ice PUBLIC OpenSSL::SSL OpenSSL::Crypto)

find_package(ZLIB REQUIRED)
target_link_libraries(ice PUBLIC ZLIB::ZLIB)

if(WIN32)
  target_link_libraries(ice PUBLIC ws2_32 mswsock)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_link_libraries(ice PUBLIC c++experimental)
endif()

install(TARGETS ice EXPORT ice
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ice DESTINATION include FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/ice DESTINATION include FILES_MATCHING PATTERN "*.hpp")
install(EXPORT ice FILE ice-targets.cmake NAMESPACE ice:: DESTINATION lib/cmake/ice)

include(CMakePackageConfigHelpers)

configure_package_config_file(config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/ice-config.cmake
  INSTALL_DESTINATION lib/cmake/ice)

write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/ice-config-version.cmake
  VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/ice-config.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/ice-config-version.cmake
  DESTINATION lib/cmake/ice)

add_library(ice::ice ALIAS ice)
