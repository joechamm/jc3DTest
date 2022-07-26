# CMakeLists.txt for jc3DCh06_VK02_DescriptorIndexing

cmake_minimum_required(VERSION 3.14)

project(jc3DCh06_VK02_DescriptorIndexing CXX C)

include_directories(${CMAKE_SOURCE_DIR}/extern/src/vulkan/include)
include_directories(${CMAKE_SOURCE_DIR}/libs/jcVkFramework/include)

add_executable(jc3DCh06_VK02_DescriptorIndexing)

if(MSVC)
	add_definitions(-D_CONSOLE)
endif()

target_sources(jc3DCh06_VK02_DescriptorIndexing 
	PUBLIC
#	${CMAKE_CURRENT_SOURCE_DIR}/src/EasyProfilerWrapper.h 
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_include_directories(jc3DCh06_VK02_DescriptorIndexing PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

set_property(TARGET jc3DCh06_VK02_DescriptorIndexing PROPERTY FOLDER "apps")

target_compile_features(jc3DCh06_VK02_DescriptorIndexing PUBLIC cxx_std_20)

target_link_libraries(jc3DCh06_VK02_DescriptorIndexing PRIVATE
	jcVkFramework
)

if(BUILD_WITH_EASY_PROFILER)
	target_link_libraries(jc3DCh06_VK02_DescriptorIndexing PRIVATE easy_profiler)
endif()