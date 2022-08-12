# CMakeLists.txt for jc3DCh09_VK01_Physics
cmake_minimum_required(VERSION 3.14)

project(jc3DCh08_VK01_ShadowMapping CXX C)

include_directories(${CMAKE_SOURCE_DIR}/extern/src/vulkan/include)
include_directories(${CMAKE_SOURCE_DIR}/extern/src/imgui)
include_directories(${CMAKE_SOURCE_DIR}/libs/jcVkFramework/include)

add_executable(jc3DCh09_VK01_Physics)

set_property(TARGET jc3DCh09_VK01_Physics PROPERTY FOLDER "apps")

target_compile_features(jc3DCh09_VK01_Physics PUBLIC cxx_std_20)

target_sources(jc3DCh09_VK01_Physics 
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/src/Physics.h
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src/Physics.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_include_directories(jc3DCh09_VK01_Physics PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(jc3DCh09_VK01_Physics PRIVATE 
	jcVkFramework
	Bullet
)

if(BUILD_WITH_EASY_PROFILER)
	target_link_libraries(jc3DCh09_VK01_Physics PRIVATE easy_profiler)
endif()