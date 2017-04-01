CMAKE_MINIMUM_REQUIRED(VERSION 2.8.3)
project(GLSL)

FILE(GLOB cmakes ${CMAKE_SOURCE_DIR}/cmake/*.cmake)
FOREACH(cmake ${cmakes})
	INCLUDE(${cmake})
ENDFOREACH(cmake)

MESSAGE(STATUS "stdgl: ${stdgl_libraries}")

ADD_SUBDIRECTORY(src)

IF (EXISTS ${CMAKE_SOURCE_DIR}/sln/CMakeLists.txt)
	ADD_SUBDIRECTORY(sln)
ENDIF()
