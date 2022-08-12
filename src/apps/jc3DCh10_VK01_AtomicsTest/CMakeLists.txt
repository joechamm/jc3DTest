# CMakeLists.txt for jc3DCh10_VK01_AtomicsTest
cmake_minimum_required(VERSION 3.14)

project(jc3DCh10_VK01_AtomicsTest CXX C)

include_directories(${CMAKE_SOURCE_DIR}/extern/src/vulkan/include)
include_directories(${CMAKE_SOURCE_DIR}/extern/src/imgui)
include_directories(${CMAKE_SOURCE_DIR}/libs/jcVkFramework/include)

add_executable(jc3DCh10_VK01_AtomicsTest)

set_property(TARGET jc3DCh10_VK01_AtomicsTest PROPERTY FOLDER "apps")

target_compile_features(jc3DCh10_VK01_AtomicsTest PUBLIC cxx_std_20)

target_sources(jc3DCh10_VK01_AtomicsTest 
	PUBLIC
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_include_directories(jc3DCh10_VK01_AtomicsTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(jc3DCh10_VK01_AtomicsTest PRIVATE 
	jcVkFramework
)

if(BUILD_WITH_EASY_PROFILER)
	target_link_libraries(jc3DCh10_VK01_AtomicsTest PRIVATE easy_profiler)
endif()