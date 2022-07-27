cmake_minimum_required(VERSION 2.8...3.20)

project(jcGLframework 
  VERSION 0.1.0
  LANGUAGES CXX
  DESCRIPTION "Library of helper functions and classes for simple OpenGL Applications. Derived mainly from 3D Graphics Rendering Cookbook"
)

# Debugging cmake
include(CMakePrintHelpers)

message(STATUS "Building ${PROJECT_NAME}")

# Include any of our cmake helpers
include(${CMAKE_SOURCE_DIR}/cmake/setup-helpers.cmake)

# Not sure if these are needed to be included here
include_directories(${CMAKE_SOURCE_DIR}/extern/src/imgui)

file(GLOB_RECURSE SRC_FILES LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.c??)
file(GLOB_RECURSE HEADER_FILES LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.h)

cmake_print_variables(CMAKE_CURRENT_SOURCE_DIR)

message(STATUS "Checking file globs...")

cmake_print_variables(SRC_FILES)
cmake_print_variables(HEADER_FILES)

SETUP_GROUPS("${HEADER_FILES}")
SETUP_GROUPS("${SRC_FILES}")

add_library(jcGLframework)

# export all our symbols for now
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
# TODO: refine our exports as in Chapter 3 of CMake Best Practices
# set_property(TARGET jcGLframework PROPERTY CXX_VISIBILITY_PRESET "hidden")
# set_property(TARGET jcGLframework PROPERTY VISIBILITY_INLINES_HIDDEN TRUE)
# include(GenerateExportHeader)
# generate_export_header(jcGLframework EXPORT_FILE_NAME export/jcGLframework/jcGLframework.hpp)
# target_include_directories(jcGLframework PUBLIC "${CMAKE_CURRENT_BINARY_DIR} /export")

target_sources(jcGLframework PRIVATE ${SRC_FILES} ${HEADER_FILES})

target_compile_features(jcGLframework PUBLIC cxx_std_20)

target_include_directories(jcGLframework 
  PRIVATE 
  ${CMAKE_CURRENT_SOURCE_DIR}/src 
  ${CMAKE_CURRENT_SOURCE_DIR}/src/glFramework
  ${CMAKE_CURRENT_SOURCE_DIR}/src/scene
  ${CMAKE_CURRENT_SOURCE_DIR}/include/jcGLframework
  ${CMAKE_CURRENT_SOURCE_DIR}/include/jcGLframework/glFramework
  ${CMAKE_CURRENT_SOURCE_DIR}/include/jcGLframework/scene
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
#	${CMAKE_CURRENT_SOURCE_DIR}/include/jcGLframework
#	${CMAKE_CURRENT_SOURCE_DIR}/include/jcGLframework/scene
#	${CMAKE_CURRENT_SOURCE_DIR}/include/jcGLframework/glFramework	
	)

target_link_libraries(jcGLframework 
  PUBLIC 
  glad
  glfw 
  assimp
)

if(BUILD_WITH_EASY_PROFILER)
	target_link_libraries(jcGLframework PUBLIC easy_profiler)
endif()

if(BUILD_WITH_OPTICK)
	target_link_libraries(jcGLframework PUBLIC OptickCore)
endif()

# Create virtual folders for Visual Studio
if(MSVC_IDE)
	# Macro to preserve source files hierarchy in the IDE
	macro(GroupSources curdir)
		file(GLOB children RELATIVE ${PROJECT_SOURCE_DIR}/${curdir} ${PROJECT_SOURCE_DIR}/${curdir}/*)
		
		foreach(child ${children})
			if(IS_DIRECTORY ${PROJECT_SOURCE_DIR}/${curdir}/${child})
				GroupSources(${curdir}/${child})
			else() 
				string(REPLACE "/" "\\" groupname ${curdir})
				string(REPLACE "src" "Sources" groupname ${groupname})
				source_group(${groupname} FILES ${PROJECT_SOURCE_DIR}/${curdir}/${child})
			endif()
		endforeach()
	endmacro()
	
	# Run Macro
	GroupSources(src)
	GroupSources(include/jcGLframework)
endif()
