cmake_minimum_required(VERSION 3.14)

project(jc3DCh02_EasyProfiler CXX C)

add_executable(jc3DCh02_EasyProfiler)

set_property(TARGET jc3DCh02_EasyProfiler PROPERTY FOLDER "apps")

include_directories(${CMAKE_SOURCE_DIR}/extern/src/easy_profiler/include)

add_definitions(-DBUILD_WITH_EASY_PROFILER=1)

target_compile_features(jc3DCh02_EasyProfiler PRIVATE cxx_std_20)

target_sources(jc3DCh02_EasyProfiler PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_link_libraries(jc3DCh02_EasyProfiler PRIVATE
	glad
	glfw
	easy_profiler
)