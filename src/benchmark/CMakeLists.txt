option(BUILD_BENCHMARK "Build benchmark" OFF)
if(NOT BUILD_BENCHMARK)
  return()
endif()

file(GLOB sources CONFIGURE_DEPENDS *.hpp *.cpp)

add_executable(benchmark EXCLUDE_FROM_ALL ${sources})
target_link_libraries(benchmark PUBLIC ice::ice)

find_package(benchmark REQUIRED)
target_link_libraries(benchmark PUBLIC benchmark::benchmark_main)
