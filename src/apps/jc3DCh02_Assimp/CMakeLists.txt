cmake_minimum_required(VERSION 3.14)

project(jc3DCh02_Assimp CXX C)

add_executable(jc3DCh02_Assimp)

set_property(TARGET jc3DCh02_Assimp PROPERTY FOLDER "apps")

target_compile_features(jc3DCh02_Assimp PRIVATE cxx_std_20)

target_sources(jc3DCh02_Assimp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

#if(MSVC)
#	set_property(TARGET jc3DCh02_ex07 PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
#endif()

target_link_libraries(jc3DCh02_Assimp PRIVATE 
	glad::glad 
	glfw
	glm::glm 
	assimp::assimp)