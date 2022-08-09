# CMakeLists.txt for jcEigenFluids

cmake_minimum_required(VERSION 3.14)

project(jcEigenFluids CXX C)

include_directories(${CMAKE_SOURCE_DIR}/libs/jcGLframework/include)
include_directories(${CMAKE_SOURCE_DIR}/extern/src/imgui)
include_directories(${CMAKE_SOURCE_DIR}/libs/jcFluidSim/include)

add_executable(jcEigenFluids)

set_property(TARGET jcEigenFluids PROPERTY FOLDER "apps")

target_compile_features(jcEigenFluids PUBLIC cxx_std_20)

target_sources(jcEigenFluids 
	PUBLIC
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_include_directories(jcEigenFluids PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_include_directories(jcEigenFluids PRIVATE ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include/jcGLframework ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include/jcGLframework/scene ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include/jcGLframework/glFramework)

target_link_libraries(jcEigenFluids PRIVATE
	jcGLframework 
	jcFluidSim
)

if(BUILD_WITH_EASY_PROFILER)
	target_link_libraries(jcEigenFluids PRIVATE easy_profiler)
endif()