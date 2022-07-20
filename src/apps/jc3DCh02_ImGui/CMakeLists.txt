cmake_minimum_required(VERSION 3.14)

project(jc3DCh02_ImGui CXX C)

add_executable(jc3DCh02_ImGui)

set_property(TARGET jc3DCh02_ImGui PROPERTY FOLDER "apps")

target_compile_features(jc3DCh02_ImGui PRIVATE cxx_std_20)

target_sources(jc3DCh02_ImGui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_link_libraries(jc3DCh02_ImGui PRIVATE 
	glad
	glfw
	imgui)