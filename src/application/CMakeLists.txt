option(BUILD_APPLICATION "Build application")
if(NOT BUILD_APPLICATION)
  return()
endif()

file(GLOB_RECURSE sources CONFIGURE_DEPENDS *.hpp *.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "src" FILES ${sources})

add_executable(application ${sources})
target_link_libraries(application PUBLIC ice::ssh)
set_target_properties(application PROPERTIES OUTPUT_NAME ice)
set_target_properties(application PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

if(WIN32)
  configure_file(main.rc.in ${CMAKE_CURRENT_BINARY_DIR}/main.rc)
  source_group("res" FILES ${CMAKE_CURRENT_BINARY_DIR}/main.rc main.manifest)
  target_sources(application PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/main.rc main.manifest)
endif()

install(TARGETS application RUNTIME DESTINATION bin)

set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT application)
