cmake_minimum_required(VERSION 3.14)

project(jc3DCh02_ex03 CXX C)

add_executable(jc3DCh02_ex03)

set_property(TARGET jc3DCh02_ex03 PROPERTY FOLDER "apps")

target_compile_features(jc3DCh02_ex03 PRIVATE cxx_std_20)

target_sources(jc3DCh02_ex03 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

#if(MSVC)
#	set_property(TARGET jc3DCh02_ex03 PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
#endif()

target_link_libraries(jc3DCh02_ex03 PRIVATE 
	glad::glad 
	glfw
	glm::glm)