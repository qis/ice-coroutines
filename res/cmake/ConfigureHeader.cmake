include_guard(GLOBAL)

macro(configure_header header source output)
  get_filename_component(header_file ${header} ABSOLUTE)
  get_filename_component(source_file ${source} ABSOLUTE)
  get_filename_component(output_file ${output} ABSOLUTE)
  
  if (EXISTS ${output_file} AND
      ${output_file} IS_NEWER_THAN ${header_file} AND 
      ${output_file} IS_NEWER_THAN ${source_file})
    return()
  endif()
  
  configure_file(${source_file} ${CMAKE_CURRENT_BINARY_DIR}/config.cpp @ONLY)
  
  try_compile(CONFIG_RESULT ${CMAKE_BINARY_DIR}
    SOURCES ${CMAKE_CURRENT_BINARY_DIR}/config.cpp
    CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON
    OUTPUT_VARIABLE CONFIG_OUTPUT)

  string(REPLACE "\n" ";" CONFIG_OUTPUT "${CONFIG_OUTPUT}")
  set(CONFIG_REGEX ".*check<([^,>]+), ?([0-9]+), ?([0-9]+)>.*")

  foreach(CONFIG_LINE ${CONFIG_OUTPUT})
    if(CONFIG_LINE MATCHES "${CONFIG_REGEX}")
      string(REGEX REPLACE "${CONFIG_REGEX}" "\\1;\\2;\\3" CONFIG_ENTRY ${CONFIG_LINE})
      list(GET CONFIG_ENTRY 0 CONFIG_TYPE)
      if(NOT ${CONFIG_TYPE}_size AND NOT ${CONFIG_TYPE}_alignment)
        list(GET CONFIG_ENTRY 1 ${CONFIG_TYPE}_size)
        list(GET CONFIG_ENTRY 2 ${CONFIG_TYPE}_alignment)
        set(${CONFIG_TYPE}_size "${${CONFIG_TYPE}_size}" CACHE STRING "")
        set(${CONFIG_TYPE}_alignment "${${CONFIG_TYPE}_alignment}" CACHE STRING "")
        if(NOT CMAKE_REQUIRED_QUIET)
          message(STATUS "Checking ${CONFIG_TYPE}: ${${CONFIG_TYPE}_size} ${${CONFIG_TYPE}_alignment}")
        endif()
      endif()
    endif()
  endforeach()

  configure_file(${header_file} ${output_file} ${ARGN})
endmacro()
