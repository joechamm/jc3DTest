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


find_package(glad CONFIG REQUIRED)
if(glad_FOUND)
	message(STATUS "Found GLAD")
else()
	message(STATUS "GLAD NOT FOUND!")
endif()
find_package(glfw3 CONFIG REQUIRED)
if(glfw3_FOUND)
	message(STATUS "Found GLFW3")
else()
	message(STATUS "GLFW3 NOT FOUND!")
endif()
find_package(volk CONFIG REQUIRED)
if(volk_FOUND)
	message(STATUS "Found VOLK")
else()
	message(STATUS "VOLK NOT FOUND!")
endif()
find_package(glslang CONFIG REQUIRED)
if(glslang_FOUND)
	message(STATUS "Found GLSLANG")
else()
	message(STATUS "GLSLANG NOT FOUND!")
endif()
find_package(assimp CONFIG REQUIRED)
if(assimp_FOUND)
	message(STATUS "Found ASSIMP")
else()
	message(STATUS "ASSIMP NOT FOUND!")
endif()
find_package(glm CONFIG REQUIRED)
if(glm_FOUND)
	message(STATUS "Found GLM")
else()
	message(STATUS "GLM NOT FOUND!")
endif()
find_package(imgui CONFIG REQUIRED)
if(imgui_FOUND)
	message(STATUS "Found IMGUI")
else() 
	message(STATUS "IMGUI NOT FOUND!") 
endif()
find_package(taskflow CONFIG REQUIRED) 
if(taskflow_FOUND)
	message(STATUS "Found TASKFLOW")
else() 
	message(STATUS "TASKFLOW NOT FOUND!")
endif() 

add_subdirectory(src/libs)
add_subdirectory(src/apps/jc3DCh02_ex03)
add_subdirectory(src/apps/jc3DCh02_ex04)
add_subdirectory(src/apps/jc3DCh02_ex07)
add_subdirectory(src/apps/jc3DCh03_GL01ApiWrapping)
add_subdirectory(src/apps/jc3DCh03_GL02VtxPulling)
add_subdirectory(src/apps/jc3DCh03_GL03CubeMap)