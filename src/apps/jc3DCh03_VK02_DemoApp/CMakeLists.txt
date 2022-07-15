# CMakeLists.txt for jc3DCh03_VK02_DemoApp

cmake_minimum_required(VERSION 3.14)

project(jc3DCh03_VK02_DemoApp CXX C)

add_executable(jc3DCh03_VK02_DemoApp)

set_property(TARGET jc3DCh03_VK02_DemoApp PROPERTY FOLDER "apps")

target_compile_features(jc3DCh03_VK02_DemoApp PUBLIC cxx_std_20)

target_sources(jc3DCh03_VK02_DemoApp 
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/src/Utils.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/UtilsVulkan.h 
	${CMAKE_CURRENT_SOURCE_DIR}/src/Utils.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/UtilsVulkan.cpp 
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_sources(jc3DCh03_VK02_DemoApp PRIVATE ${CMAKE_SOURCE_DIR}/extern/src/glslang/StandAlone/ResourceLimits.cpp)

target_include_directories(jc3DCh03_VK02_DemoApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(jc3DCh03_VK02_DemoApp PRIVATE 
	glfw
	volk
	glslang
	SPIRV
	assimp
)