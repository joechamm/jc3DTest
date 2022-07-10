cmake_minimum_required(VERSION 3.14)
project(jc3DTest
	VERSION 0.1.0
	DESCRIPTION "A test to get CMake and Vcpkg to work to build project chapters for 3D Graphics Rendering Cookbook"
	LANGUAGES CXX C)
	
# Debugging cmake
include(CMakePrintHelpers)

message(STATUS "Building ${PROJECT_NAME}")
	
# use folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(WIN32)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

if(MSVC)
	add_definitions(-D_CONSOLE)
endif()

option(BUILD_SHARED_LIBS "Build Shared Libraries" ON)
# TODO: configure externals

set(LIB_TYPE STATIC)

if(BUILD_SHARED_LIBS)
	set(LIB_TYPE SHARED)
endif()

# Configure assets header file 
configure_file(src/helpers/RootDir.h.in src/helpers/RootDir.h)
include_directories(${CMAKE_BINARY_DIR}/src)

add_subdirectory(src/libs)
add_subdirectory(src/apps/jc3DCh02_ex07)