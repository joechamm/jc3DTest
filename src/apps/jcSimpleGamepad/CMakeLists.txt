# CMakeLists.txt for jcSimpleGamepad

cmake_minimum_required(VERSION 3.14)

project(jcSimpleGamepad CXX C)

include_directories(${CMAKE_SOURCE_DIR}/libs/jcGLframework/include)
include_directories(${CMAKE_SOURCE_DIR}/extern/src/imgui)

add_executable(jcSimpleGamepad)

set_property(TARGET jcSimpleGamepad PROPERTY FOLDER "apps")

target_compile_features(jcSimpleGamepad PUBLIC cxx_std_20)

target_sources(jcSimpleGamepad 
	PUBLIC
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_include_directories(jcSimpleGamepad PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_include_directories(jcSimpleGamepad PRIVATE ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include/jcGLframework ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include/jcGLframework/scene ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include/jcGLframework/glFramework)

target_link_libraries(jcSimpleGamepad PRIVATE
	jcGLframework 
)

if(BUILD_WITH_EASY_PROFILER)
	target_link_libraries(jcSimpleGamepad PRIVATE easy_profiler)
endif()