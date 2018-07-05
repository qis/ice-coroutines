option(BUILD_TESTS "Build tests" ${BUILD_TESTING})
if(NOT BUILD_TESTS)
  return()
endif()

include(GoogleTest)

file(GLOB sources CONFIGURE_DEPENDS *.hpp *.cpp)
source_group("" FILES ${sources})

add_executable(tests ${sources})
target_link_libraries(tests PUBLIC ice)

find_package(GTest REQUIRED)
target_link_libraries(tests PUBLIC GTest::Main)
gtest_add_tests(TARGET tests SOURCES ${sources} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
