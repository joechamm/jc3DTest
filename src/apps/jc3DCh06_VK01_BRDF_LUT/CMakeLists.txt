# CMakeLists.txt for jc3DCh06_VK01_BRDF_LUT

cmake_minimum_required(VERSION 3.14)

project(jc3DCh06_VK01_BRDF_LUT CXX C)

add_executable(jc3DCh06_VK01_BRDF_LUT)

set_property(TARGET jc3DCh06_VK01_BRDF_LUT PROPERTY FOLDER "apps")

target_compile_features(jc3DCh06_VK01_BRDF_LUT PUBLIC cxx_std_20)

target_sources(jc3DCh06_VK01_BRDF_LUT 
	PUBLIC
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_sources(jc3DCh06_VK01_BRDF_LUT PRIVATE ${CMAKE_SOURCE_DIR}/extern/src/glslang/StandAlone/ResourceLimits.cpp)

target_include_directories(jc3DCh06_VK01_BRDF_LUT PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers ${CMAKE_CURRENT_SOURCE_DIR}/src/vkFramework)

target_link_libraries(jc3DCh06_VK01_BRDF_LUT PRIVATE 
	jcVkFramework
)

if(BUILD_WITH_EASY_PROFILER)
	target_link_libraries(jc3DCh06_VK01_BRDF_LUT PRIVATE easy_profiler)
endif()