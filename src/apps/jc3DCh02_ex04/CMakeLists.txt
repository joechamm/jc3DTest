cmake_minimum_required(VERSION 3.14)

project(jc3DCh02_ex04 CXX C)

add_executable(jc3DCh02_ex04)

set_property(TARGET jc3DCh02_ex04 PROPERTY FOLDER "apps")

target_compile_features(jc3DCh02_ex04 PRIVATE cxx_std_20)

target_sources(jc3DCh02_ex04 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

#if(MSVC)
#	set_property(TARGET jc3DCh02_ex04 PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
#endif()

target_link_libraries(jc3DCh02_ex04 PRIVATE 
	glad::glad 
	glfw
	glm::glm 
	imgui::imgui)