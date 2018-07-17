option(BUILD_BENCHMARK "Build benchmark" OFF)
if(NOT BUILD_BENCHMARK)
  return()
endif()

file(GLOB sources CONFIGURE_DEPENDS *.hpp *.cpp)
source_group("" FILES ${sources})

add_executable(benchmark EXCLUDE_FROM_ALL ${sources})
target_link_libraries(benchmark PUBLIC ice::net)

find_package(benchmark REQUIRED)
target_link_libraries(benchmark PUBLIC benchmark::benchmark_main)

if(CMAKE_GENERATOR MATCHES "Visual Studio")
  add_custom_target(RUN_BENCHMARK COMMAND $<TARGET_FILE:benchmark> WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
  set_target_properties(RUN_BENCHMARK PROPERTIES FOLDER "build")
endif()

set_target_properties(benchmark PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
