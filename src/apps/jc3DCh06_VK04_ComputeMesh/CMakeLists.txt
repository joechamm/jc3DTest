# CMakeLists.txt for jc3DCh06_VK04_ComputeMesh
cmake_minimum_required(VERSION 3.14)

project(jc3DCh06_VK04_ComputeMesh CXX C)

include_directories(${CMAKE_SOURCE_DIR}/extern/src/vulkan/include)
include_directories(${CMAKE_SOURCE_DIR}/extern/src/imgui)
include_directories(${CMAKE_SOURCE_DIR}/libs/jcVkFramework/include)

add_executable(jc3DCh06_VK04_ComputeMesh)

set_property(TARGET jc3DCh06_VK04_ComputeMesh PROPERTY FOLDER "apps")

target_compile_features(jc3DCh06_VK04_ComputeMesh PUBLIC cxx_std_20)

target_sources(jc3DCh06_VK04_ComputeMesh 
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_include_directories(jc3DCh06_VK04_ComputeMesh PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(jc3DCh06_VK04_ComputeMesh PRIVATE 
	jcVkFramework
)

if(BUILD_WITH_EASY_PROFILER)
	target_link_libraries(jc3DCh06_VK04_ComputeMesh PRIVATE easy_profiler)
endif()