# CMakeLists.txt for jcEulerFluids3D

cmake_minimum_required(VERSION 3.14)

project(jcEulerFluids3D CXX C)

include_directories(${CMAKE_SOURCE_DIR}/libs/jcGLframework/include)
include_directories(${CMAKE_SOURCE_DIR}/extern/src/imgui)
include_directories(${CMAKE_SOURCE_DIR}/libs/jcFluidSim/include)

add_executable(jcEulerFluids3D)

set_property(TARGET jcEulerFluids3D PROPERTY FOLDER "apps")

target_compile_features(jcEulerFluids3D PUBLIC cxx_std_20)

target_sources(jcEulerFluids3D 
	PUBLIC
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_include_directories(jcEulerFluids3D PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_include_directories(jcEulerFluids3D PRIVATE ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include/jcGLframework ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include/jcGLframework/scene ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include/jcGLframework/glFramework)

target_link_libraries(jcEulerFluids3D PRIVATE
	jcGLframework 
)

if(BUILD_WITH_EASY_PROFILER)
	target_link_libraries(jcEulerFluids3D PRIVATE easy_profiler)
endif()