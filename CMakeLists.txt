cmake_minimum_required(VERSION 3.18.0 FATAL_ERROR)
project(ice VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/res/cmake)

add_subdirectory(src/ice)
add_subdirectory(src/serial)
add_subdirectory(src/ssh)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/benchmark/CMakeLists.txt)
  add_subdirectory(src/benchmark)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/tests/CMakeLists.txt)
  enable_testing()
  add_subdirectory(src/tests)
endif()
