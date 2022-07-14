cmake_minimum_required(VERSION 2.8...3.20)
project(jc3DTest
	VERSION 0.2.0
	DESCRIPTION "A test to get CMake and Vcpkg to work to build project chapters for 3D Graphics Rendering Cookbook"
	LANGUAGES CXX C)
	
# Debugging cmake
include(CMakePrintHelpers)

message(STATUS "Building ${PROJECT_NAME}")

option(BUILD_WITH_EASY_PROFILER "Enable EasyProfiler usage" OFF)
option(BUILD_WITH_OPTICK "Enable Optick usage" OFF)

set(EASY_PROFILER_NO_GUI ON CACHE BOOL "")
set(EASY_PROFILER_NO_SAMPLES ON CACHE BOOL "")
	
# use folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
set(GLFW_INSTALL OFF CACHE BOOL "")

set(ASSIMP_NO_EXPORT ON CACHE BOOL "")
set(ASSIMP_BUILD_DRACO OFF CACHE BOOL "")
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "")
set(ASSIMP_BUILD_TESTS ON CACHE BOOL "")
set(ASSIMP_INSTALL_PDB OFF CACHE BOOL "")
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "")
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "")
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "")

set(MESHOPT_BUILD_DEMO OFF CACHE BOOL "")
set(MESHOPT_BUILD_TOOLS OFF CACHE BOOL "")
set(MESHOPT_BUILD_SHARED_LIBS OFF CACHE BOOL "")

set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "")
set(ENABLE_HLSL OFF CACHE BOOL "")
set(ENABLE_CTEST OFF CACHE BOOL "")
set(ENABLE_OPT OFF CACHE BOOL "")
set(SKIP_GLSLANG_INSTALL ON CACHE BOOL "")

# external dependencies
add_subdirectory(extern/src/etc2comp)

include_directories(extern/src/etc2comp/EtcLib/Etc)
include_directories(extern/src/etc2comp/EtcLib/EtcCodec)

set_property(TARGET EtcLib 	PROPERTY FOLDER "ThirdPartyLibraries")
set_property(TARGET EtcTool	PROPERTY FOLDER "ThirdPartyLibraries")

if(BUILD_WITH_EASY_PROFILER)
	message("Enabled EasyProfiler")	
	add_subdirectory(extern/src/easy_sources)
	add_definitions(-DBUILD_WITH_EASY_PROFILER=1)
	include_directories(extern/src/easy_profiler/include)
	set_property(TARGET easy_profiler PROPERTY FOLDER "ThirdPartyLibraries")
#	set_property(TARGET profile_converter PROPERTY FOLDER "ThirdPartyLibraries")
endif()
if(BUILD_WITH_OPTICK)
	message("Enabled Optick")
	add_subdirectory(extern/src/optick)
	add_definitions(-DBUILD_WITH_OPTICK=1)
	set_property(TARGET OptickCore PROPERTY FOLDER "ThirdPartyLibraries")
endif()

if(WIN32)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_subdirectory(extern/src/glfw)
add_subdirectory(extern/src/assimp)
add_subdirectory(extern/src/meshoptimizer)
add_subdirectory(extern/src/volk) 
add_subdirectory(extern/src/glslang)

# Configure assets header file 
configure_file(src/helpers/RootDir.h.in src/helpers/RootDir.h)
include_directories(${CMAKE_BINARY_DIR}/src)

include_directories(.)
include_directories(extern/src)
include_directories(extern/src/glfw/include)
include_directories(extern/src/glm)
include_directories(extern/src/gli)
include_directories(extern/src/glad/include)
include_directories(extern/src/assimp/include)
include_directories(extern/src/glslang/glslang/Include)
include_directories(extern/src/taskflow)
include_directories(extern/src/rapidjson/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/extern/src/assimp/include)
include_directories(extern/src/stb)
include_directories(extern/src/glslang)
include_directories(extern/src/glslang/StandAlone)
include_directories(extern/src/glslang/src)

set(GLAD_SOURCES extern/src/glad/gl.c extern/src/glad/vulkan.c)
if(WIN32)
	set(GLAD_SOURCES ${GLAD_SOURCES} extern/src/glad/src/wgl.c)
endif()

add_library(glad ${GLAD_SOURCES}) 

set_property(TARGET glfw PROPERTY FOLDER "ThirdPartyLibraries")
set_property(TARGET assimp PROPERTY FOLDER "ThirdPartyLibraries")
set_property(TARGET glad PROPERTY FOLDER "ThirdPartyLibraries")
set_property(TARGET meshoptimizer PROPERTY FOLDER "ThirdPartyLibraries")
set_property(TARGET volk PROPERTY FOLDER "ThirdPartyLibraries")
if(WIN32)
#	set_property(TARGET zlib PROPERTY FOLDER "ThirdPartyLibraries")
	set_property(TARGET zlibstatic	PROPERTY FOLDER "ThirdPartyLibraries")
	set_property(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTY FOLDER "ThirdPartyLibraries")
endif()
set_property(TARGET glslang PROPERTY FOLDER "ThirdPartyLibraries")
set_property(TARGET OGLCompiler PROPERTY FOLDER "ThirdPartyLibraries")
set_property(TARGET OSDependent PROPERTY FOLDER "ThirdPartyLibraries")
set_property(TARGET SPIRV PROPERTY FOLDER "ThirdPartyLibraries")
set_property(TARGET SPVRemapper PROPERTY FOLDER "ThirdPartyLibraries")

add_subdirectory(src/libs)

add_subdirectory(src/apps/jc3DCh02_Assimp)
if(BUILD_WITH_EASY_PROFILER)
	add_subdirectory(src/apps/jc3DCh02_EasyProfiler)
endif()
add_subdirectory(src/apps/jc3DCh02_ETC2Comp)
add_subdirectory(src/apps/jc3DCh02_ImGui)
add_subdirectory(src/apps/jc3DCh02_MeshOptimizer)
if(BUILD_WITH_OPTICK)
	add_subdirectory(src/apps/jc3DCh02_Optick)
endif()
add_subdirectory(src/apps/jc3DCh02_STB)
add_subdirectory(src/apps/jc3DCh02_Taskflow)
add_subdirectory(src/apps/jc3DCh03_GL01ApiWrapping)
add_subdirectory(src/apps/jc3DCh03_GL02VtxPulling)
add_subdirectory(src/apps/jc3DCh03_GL03CubeMap)
add_subdirectory(src/apps/jc3DCh03_VK01_GLSLang)
add_subdirectory(src/apps/jc3DCh03_VK02_DemoApp)