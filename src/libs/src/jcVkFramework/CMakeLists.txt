cmake_minimum_required(VERSION 2.8...3.20)

project(jcVkFramework 
  VERSION 0.1.0
  PROJECT_DESCRIPTION "Library of helper functions and classes for simple Vulkan Applications. Derived mainly from 3D Graphics Rendering Cookbook"
  LANGUAGES CXX)

# Debugging cmake
include(CMakePrintHelpers)

message(STATUS "Building ${PROJECT_NAME}")

# Include any of our cmake helpers
include(${CMAKE_SOURCE_DIR}/cmake/setup-helpers.cmake)

file(GLOB_RECURSE SRC_FILES LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.c??)
file(GLOB_RECURSE HEADER_FILES LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.h)

cmake_print_variables(CMAKE_CURRENT_SOURCE_DIR)

message(STATUS "Checking file globs...")

cmake_print_variables(SRC_FILES)
cmake_print_variables(HEADER_FILES);

SETUP_GROUPS("${HEADER_FILES}")
SETUP_GROUPS("${SRC_FILES}")

add_library(jcVkFramework)

target_sources(jcVkFramework PRIVATE ${SRC_FILES} ${HEADER_FILES})

# include StandAlone ResourceLimits.cpp
target_sources(jcVkFramework PRIVATE ${CMAKE_SOURCE_DIR}/extern/src/glslang/StandAlone/ResourceLimits.cpp)

target_compile_features(jcVkFramework PUBLIC cxx_std_20)

target_include_directories(jcVkFramework 
  PRIVATE 
  ${CMAKE_CURRENT_SOURCE_DIR}/src 
  ${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers 
  ${CMAKE_CURRENT_SOURCE_DIR}/src/vkFramework)

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
	GroupSources(include/jc3DTestSharedLibs)
endif()

target_link_libraries(jcVkFramework 
  PRIVATE 
  glad
  glfw
  volk
  glslang
  SPIRV
  assimp
  )