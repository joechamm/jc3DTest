# CMakeLists.txt for jc3DCh04_VK01_DemoApp

cmake_minimum_required(VERSION 3.14)

project(jc3DCh04_VK01_DemoApp CXX C)

add_executable(jc3DCh04_VK01_DemoApp)

set_property(TARGET jc3DCh04_VK01_DemoApp PROPERTY FOLDER "apps")

target_compile_features(jc3DCh04_VK01_DemoApp PUBLIC cxx_std_20)

target_sources(jc3DCh04_VK01_DemoApp 
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/src/Utils.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/UtilsVulkan.h 
	${CMAKE_CURRENT_SOURCE_DIR}/src/UtilsMath.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/UtilsFPS.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/UtilsCubemap.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/Graph.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/Camera.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/debug.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/Bitmap.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/ResourceString.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers/VulkanCanvas.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers/VulkanClear.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers/VulkanCube.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers/VulkanFinish.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers/VulkanImGui.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers/VulkanModelRenderer.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers/VulkanRendererBase.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/EasyProfilerWrapper.h 
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers/VulkanCanvas.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers/VulkanClear.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers/VulkanCube.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers/VulkanFinish.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers/VulkanImGui.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers/VulkanModelRenderer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers/VulkanRendererBase.cpp	
	${CMAKE_CURRENT_SOURCE_DIR}/src/Utils.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/UtilsVulkan.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/UtilsCubemap.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_sources(jc3DCh04_VK01_DemoApp PRIVATE ${CMAKE_SOURCE_DIR}/extern/src/glslang/StandAlone/ResourceLimits.cpp)

target_sources(jc3DCh04_VK01_DemoApp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_wrapper.cpp)

#target_sources(jc3DCh04_VK01_DemoApp PUBLIC ${CMAKE_SOURCE_DIR}/extern/src/imgui/imgui.h PRIVATE ${CMAKE_SOURCE_DIR}/extern/src/imgui/imgui.cpp)

target_include_directories(jc3DCh04_VK01_DemoApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/src/vkRenderers)

target_link_libraries(jc3DCh04_VK01_DemoApp PRIVATE 
	glfw
	volk
	glslang
	SPIRV
	assimp
)

if(BUILD_WITH_EASY_PROFILER)
	target_link_libraries(jc3DCh04_VK01_DemoApp PRIVATE easy_profiler)
endif()

if(BUILD_WITH_OPTICK)
	target_link_libraries(jc3DCh04_VK01_DemoApp PRIVATE OptickCore)
endif()