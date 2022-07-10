cmake_minimum_required(VERSION 3.14)

project(jc3DTestSharedLibs CXX C)

# Debugging cmake
include(CMakePrintHelpers)

message(STATUS "Building ${PROJECT_NAME}")

include(${CMAKE_SOURCE_DIR}/cmake/setup-helpers.cmake)

file(GLOB_RECURSE SRC_FILES LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.c??)
file(GLOB_RECURSE HEADER_FILES LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.h)

cmake_print_variables(CMAKE_CURRENT_SOURCE_DIR)

#file(GLOB GLFRAMEWORK_SRC_FILES LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/src/jc3DTestSharedLibs/glFramework *.cpp)
#file(GLOB GLFRAMEWORK_HEADER_FILES LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/include/jc3DTestSharedLibs/glFramework *.h)
#file(GLOB SCENE_SRC_FILES LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/src/jc3DTestSharedLibs/scene *.cpp)
#file(GLOB SCENE_HEADER_FILES LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/include/jc3DTestSharedLibs/scene *.h)
#file(GLOB VKFRAMEWORK_SRC_FILES LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/src/jc3DTestSharedLibs/vkFramework *.cpp)
#file(GLOB VKFRAMEWORK_HEADER_FILES LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/include/jc3DTestSharedLibs/vkFramework *.h)
#file(GLOB VKRENDERERS_SRC_FILES LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/src/jc3DTestSharedLibs/vkRenderers *.cpp)
#file(GLOB VKRENDERERS_HEADER_FILES LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/include/jc3DTestSharedLibs/vkRenderers *.h)
#file(GLOB JCSHARED_SRC_FILES LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/src/jc3DTestSharedLibs *.cpp)
#file(GLOB JCSHARED_HEADER_FILES LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/include/jc3DTestSharedLibs *.h)

message(STATUS "Checking file globs...")
#cmake_print_variables(GLFRAMEWORK_SRC_FILES)
#cmake_print_variables(GLFRAMEWORK_HEADER_FILES)
#cmake_print_variables(SCENE_SRC_FILES)
#cmake_print_variables(SCENE_HEADER_FILES)
#cmake_print_variables(VKFRAMEWORK_SRC_FILES)
#cmake_print_variables(VKFRAMEWORK_HEADER_FILES)
#cmake_print_variables(VKRENDERERS_SRC_FILES)
#cmake_print_variables(VKRENDERERS_HEADER_FILES)
#cmake_print_variables(JCSHARED_SRC_FILES)
#cmake_print_variables(JCSHARED_HEADER_FILES)

#string(CONCAT SRC_FILES "${GLFRAMEWORK_SRC_FILES}" "${SCENE_SRC_FILES}" "${VKFRAMEWORK_SRC_FILES}" "${VKRENDERERS_SRC_FILES}" "${JCSHARED_SRC_FILES}")
#string(CONCAT HEADER_FILES "${GLFRAMEWORK_HEADER_FILES}" "${SCENE_HEADER_FILES}" "${VKFRAMEWORK_HEADER_FILES}" "${VKRENDERERS_HEADER_FILES}" "${JCSHARED_HEADER_FILES}")

cmake_print_variables(SRC_FILES)
cmake_print_variables(HEADER_FILES)

SETUP_GROUPS("${HEADER_FILES}")
SETUP_GROUPS("${SRC_FILES}")

add_library(jc3DTestSharedLibs)

target_sources(jc3DTestSharedLibs PRIVATE ${SRC_FILES} ${HEADER_FILES})

target_compile_features(jc3DTestSharedLibs PUBLIC cxx_std_20)

target_include_directories(jc3DTestSharedLibs 
	PRIVATE src/jc3DTestSharedLibs src/jc3DTestSharedLibs/glFramework src/jc3DTestSharedLibs/scene src/jc3DTestSharedLibs/vkFramework src/jc3DTestSharedLibs/vkRenderers
	PUBLIC include
)

find_package(glad CONFIG REQUIRED)
if(glad_FOUND)
	message(STATUS "Found GLAD")
else()
	message(STATUS "GLAD NOT FOUND!")
endif()
find_package(glfw3 CONFIG REQUIRED)
if(glfw3_FOUND)
	message(STATUS "Found GLFW3")
else()
	message(STATUS "GLFW3 NOT FOUND!")
endif()
find_package(volk CONFIG REQUIRED)
if(volk_FOUND)
	message(STATUS "Found VOLK")
else()
	message(STATUS "VOLK NOT FOUND!")
endif()
find_package(glslang CONFIG REQUIRED)
if(glslang_FOUND)
	message(STATUS "Found GLSLANG")
else()
	message(STATUS "GLSLANG NOT FOUND!")
endif()
find_package(assimp CONFIG REQUIRED)
if(assimp_FOUND)
	message(STATUS "Found ASSIMP")
else()
	message(STATUS "ASSIMP NOT FOUND!")
endif()
find_package(glm CONFIG REQUIRED)
if(glm_FOUND)
	message(STATUS "Found GLM")
else()
	message(STATUS "GLM NOT FOUND!")
endif()

target_link_libraries(jc3DTestSharedLibs PUBLIC 
	glad::glad 
	glfw 
	volk::volk
	volk::volk_headers
	glslang
	SPIRV
	assimp::assimp
	glm::glm
	)