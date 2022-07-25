# CMakeLists.txt for jc3DCh05_VK01_MultiMeshDraw

cmake_minimum_required(VERSION 3.14)

project(jc3DCh05_VK01_MultiMeshDraw CXX C)

add_executable(jc3DCh05_VK01_MultiMeshDraw)

set_property(TARGET jc3DCh05_VK01_MultiMeshDraw PROPERTY FOLDER "apps")

target_compile_features(jc3DCh05_VK01_MultiMeshDraw PUBLIC cxx_std_20)

target_sources(jc3DCh05_VK01_MultiMeshDraw 
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/src/Utils.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/UtilsVulkan.h 
	${CMAKE_CURRENT_SOURCE_DIR}/src/UtilsMath.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/UtilsFPS.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/UtilsCubemap.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/Camera.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/Bitmap.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/VtxData.h 
	${CMAKE_CURRENT_SOURCE_DIR}/src/debug.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/ResourceString.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers/VulkanClear.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers/VulkanFinish.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers/VulkanRendererBase.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers/VulkanMultiMeshRenderer.h 
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkFramework/VulkanApp.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkFramework/VulkanResources.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkFramework/Renderer.h 
	${CMAKE_CURRENT_SOURCE_DIR}/src/EasyProfilerWrapper.h 
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers/VulkanClear.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers/VulkanFinish.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers/VulkanRendererBase.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers/VulkanMultiMeshRenderer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkFramework/VulkanApp.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkFramework/VulkanResources.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/VtxData.cpp 
	${CMAKE_CURRENT_SOURCE_DIR}/src/Utils.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/UtilsVulkan.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/UtilsCubemap.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_sources(jc3DCh05_VK01_MultiMeshDraw PRIVATE ${CMAKE_SOURCE_DIR}/extern/src/glslang/StandAlone/ResourceLimits.cpp)

target_include_directories(jc3DCh05_VK01_MultiMeshDraw PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers ${CMAKE_CURRENT_SOURCE_DIR}/src/vkFramework)

target_link_libraries(jc3DCh05_VK01_MultiMeshDraw PRIVATE 
	glfw
	volk
	glslang
	SPIRV
	assimp
)

if(BUILD_WITH_EASY_PROFILER)
	target_link_libraries(jc3DCh05_VK01_MultiMeshDraw PRIVATE easy_profiler)
endif()