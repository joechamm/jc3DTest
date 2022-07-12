# CMakeLists.txt for jc3DCh03_VK02_DemoApp

cmake_minimum_required(VERSION 3.14)

project(jc3DCh03_VK02_DemoApp CXX C)

find_package(stb CONFIG REQUIRED)

add_executable(jc3DCh03_VK02_DemoApp)

set_property(TARGET jc3DCh03_VK02_DemoApp PROPERTY FOLDER "apps")

target_compile_features(jc3DCh03_VK02_DemoApp PRIVATE cxx_std_20)

target_sources(jc3DCh03_VK02_DemoApp PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/src/Utils.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/UtilsVulkan.h
	PRIVATE 
	${CMAKE_CURRENT_SOURCE_DIR}/src/Utils.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/UtilsVulkan.cpp 
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_include_directories(jc3DCh03_VK02_DemoApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(MSVC)
	set_property(TARGET jc3DCh03_VK02_DemoApp PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()

target_link_libraries(jc3DCh03_VK02_DemoApp PRIVATE 
	stb
	glfw
	volk::volk 
	volk::volk_headers
	glslang
	SPIRV
)