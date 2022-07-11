cmake_minimum_required(VERSION 3.14)

project(jc3DCh02_STB CXX C)

add_executable(jc3DCh02_STB)

set_property(TARGET jc3DCh02_STB PROPERTY FOLDER "apps")

target_compile_features(jc3DCh02_STB PRIVATE cxx_std_20)

target_sources(jc3DCh02_STB PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

#if(MSVC)
#	set_property(TARGET jc3DCh02_ex03 PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
#endif()

target_link_libraries(jc3DCh02_STB PRIVATE 
	glad::glad 
	glfw
	glm::glm)