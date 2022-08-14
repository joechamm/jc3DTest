# CMakeLists.txt for jcEulerFluids2D

cmake_minimum_required(VERSION 3.14)

project(jcEulerFluids2D CXX C)

include_directories(${CMAKE_SOURCE_DIR}/libs/jcGLframework/include)
include_directories(${CMAKE_SOURCE_DIR}/extern/src/imgui)
include_directories(${CMAKE_SOURCE_DIR}/libs/jcFluidSim/include)

add_executable(jcEulerFluids2D)

set_property(TARGET jcEulerFluids2D PROPERTY FOLDER "apps")

target_compile_features(jcEulerFluids2D PUBLIC cxx_std_20)

target_sources(jcEulerFluids2D 
	PUBLIC
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_include_directories(jcEulerFluids2D PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_include_directories(jcEulerFluids2D PRIVATE ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include/jcGLframework ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include/jcGLframework/scene ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include/jcGLframework/glFramework)

target_link_libraries(jcEulerFluids2D PRIVATE
	jcGLframework 
)

if(BUILD_WITH_EASY_PROFILER)
	target_link_libraries(jcEulerFluids2D PRIVATE easy_profiler)
endif()