# CMakeLists.txt for jc3DCh06_VK02_DescriptorIndexing

cmake_minimum_required(VERSION 3.14)

project(jc3DCh06_VK02_DescriptorIndexing CXX C)

include_directories(${CMAKE_SOURCE_DIR}/extern/src/vulkan/include)

add_library(jcVkFramework STATIC)

target_compile_features(jcVkFramework PUBLIC cxx_std_20)

target_sources(jcVkFramework 
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/src/Bitmap.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/Utils.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/UtilsVulkan.h 
	${CMAKE_CURRENT_SOURCE_DIR}/src/UtilsMath.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/UtilsCubemap.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/debug.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/ResourceString.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers/VulkanClear.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers/VulkanFinish.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers/VulkanRendererBase.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers/VulkanQuadRenderer.h 
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkFramework/VulkanApp.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers/VulkanClear.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers/VulkanFinish.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers/VulkanRendererBase.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers/VulkanQuadRenderer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkFramework/VulkanApp.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Utils.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/UtilsVulkan.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/UtilsCubemap.cpp
)

target_sources(jcVkFramework PRIVATE ${CMAKE_SOURCE_DIR}/extern/src/glslang/StandAlone/ResourceLimits.cpp)

target_include_directories(jcVkFramework PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers ${CMAKE_CURRENT_SOURCE_DIR}/src/vkFramework)

target_link_libraries(jcVkFramework PRIVATE 
	glfw
	volk
	glslang
	SPIRV
)

add_executable(jc3DCh06_VK02_DescriptorIndexing)

target_sources(jc3DCh06_VK02_DescriptorIndexing 
	PUBLIC
#	${CMAKE_CURRENT_SOURCE_DIR}/src/EasyProfilerWrapper.h 
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_include_directories(jc3DCh06_VK02_DescriptorIndexing PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers ${CMAKE_CURRENT_SOURCE_DIR}/src/vkFramework)

set_property(TARGET jc3DCh06_VK02_DescriptorIndexing PROPERTY FOLDER "apps")

target_compile_features(jc3DCh06_VK02_DescriptorIndexing PUBLIC cxx_std_20)

target_link_libraries(jc3DCh06_VK02_DescriptorIndexing PRIVATE
	jcVkFramework
)

#if(BUILD_WITH_EASY_PROFILER)
#	target_link_libraries(jc3DCh06_VK02_DescriptorIndexing PRIVATE easy_profiler)
#endif()