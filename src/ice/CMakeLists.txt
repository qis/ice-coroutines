file(GLOB_RECURSE sources CONFIGURE_DEPENDS *.hpp *.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "ice" FILES ${sources})

add_library(ice STATIC ${sources})

include(ConfigureHeader)
configure_header(config.hpp.in config.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/src/ice/config.hpp NEWLINE_STYLE LF)
source_group("ice" FILES ${CMAKE_CURRENT_BINARY_DIR}/src/ice/config.hpp)
target_sources(ice PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/src/ice/config.hpp)

target_include_directories(ice PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
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

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/src/ice DESTINATION include FILES_MATCHING PATTERN "*.hpp")

install(TARGETS ice EXPORT ${PROJECT_NAME}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)