# CMakeLists.txt for jcInstancedCube

cmake_minimum_required(VERSION 3.14)

project(jcInstancedCube CXX C)

include_directories(${CMAKE_SOURCE_DIR}/libs/jcGLframework/include)
include_directories(${CMAKE_SOURCE_DIR}/extern/src/imgui)

add_executable(jcInstancedCube)

set_property(TARGET jcInstancedCube PROPERTY FOLDER "apps")

target_compile_features(jcInstancedCube PUBLIC cxx_std_20)

target_sources(jcInstancedCube 
	PUBLIC
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_include_directories(jcInstancedCube PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_include_directories(jcInstancedCube PRIVATE ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include/jcGLframework ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include/jcGLframework/scene ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include/jcGLframework/glFramework)

target_link_libraries(jcInstancedCube PRIVATE
	jcGLframework 
)

if(BUILD_WITH_EASY_PROFILER)
	target_link_libraries(jcInstancedCube PRIVATE easy_profiler)
endif()