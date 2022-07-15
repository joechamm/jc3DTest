cmake_minimum_required(VERSION 3.14)

project(jc3DCh02_Taskflow CXX C)

add_executable(jc3DCh02_Taskflow)

set_property(TARGET jc3DCh02_Taskflow PROPERTY FOLDER "apps")

target_compile_features(jc3DCh02_Taskflow PRIVATE cxx_std_20)

target_sources(jc3DCh02_Taskflow PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_link_libraries(jc3DCh02_Taskflow PRIVATE 
	glad
	glfw)