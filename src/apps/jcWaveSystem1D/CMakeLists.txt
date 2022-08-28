# CMakeLists.txt for jcWaveSystem1D

cmake_minimum_required(VERSION 3.14)

project(jcWaveSystem1D CXX C)

include_directories(${CMAKE_SOURCE_DIR}/libs/jcGLframework/include)
include_directories(${CMAKE_SOURCE_DIR}/extern/src/imgui)

add_executable(jcWaveSystem1D)

set_property(TARGET jcWaveSystem1D PROPERTY FOLDER "apps")

target_compile_features(jcWaveSystem1D PUBLIC cxx_std_20)

target_sources(jcWaveSystem1D 
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/src/WaveSystem1D.h
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src/WaveSystem1D.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_include_directories(jcWaveSystem1D PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_include_directories(jcWaveSystem1D PRIVATE ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include/jcGLframework ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include/jcGLframework/scene ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include/jcGLframework/glFramework)

target_link_libraries(jcWaveSystem1D PRIVATE
	jcGLframework 
)

if(BUILD_WITH_EASY_PROFILER)
	target_link_libraries(jcWaveSystem1D PRIVATE easy_profiler)
endif()