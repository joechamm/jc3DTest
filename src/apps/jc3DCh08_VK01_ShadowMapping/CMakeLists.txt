# CMakeLists.txt for jc3DCh08_VK01_ShadowMapping
cmake_minimum_required(VERSION 3.14)

project(jc3DCh08_VK01_ShadowMapping CXX C)

include_directories(${CMAKE_SOURCE_DIR}/extern/src/vulkan/include)
include_directories(${CMAKE_SOURCE_DIR}/extern/src/imgui)
include_directories(${CMAKE_SOURCE_DIR}/libs/jcVkFramework/include)

add_executable(jc3DCh08_VK01_ShadowMapping)

set_property(TARGET jc3DCh08_VK01_ShadowMapping PROPERTY FOLDER "apps")

target_compile_features(jc3DCh08_VK01_ShadowMapping PUBLIC cxx_std_20)

target_sources(jc3DCh08_VK01_ShadowMapping 
	PUBLIC
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_include_directories(jc3DCh08_VK01_ShadowMapping PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(jc3DCh08_VK01_ShadowMapping PRIVATE 
	jcVkFramework
)

if(BUILD_WITH_EASY_PROFILER)
	target_link_libraries(jc3DCh08_VK01_ShadowMapping PRIVATE easy_profiler)
endif()