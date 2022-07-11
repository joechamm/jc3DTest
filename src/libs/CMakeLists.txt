cmake_minimum_required(VERSION 3.14)

project(jc3DTestSharedLibs CXX C)

# Debugging cmake
include(CMakePrintHelpers)

message(STATUS "Building ${PROJECT_NAME}")

include(${CMAKE_SOURCE_DIR}/cmake/setup-helpers.cmake)

file(GLOB_RECURSE SRC_FILES LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.c??)
file(GLOB_RECURSE HEADER_FILES LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.h)

cmake_print_variables(CMAKE_CURRENT_SOURCE_DIR)

message(STATUS "Checking file globs...")

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

# Create virtual folders for Visual Studio
if(MSVC_IDE)
	# Macro to preserve source files hierarchy in the IDE
	macro(GroupSources curdir)
		file(GLOB children RELATIVE ${PROJECT_SOURCE_DIR}/${curdir} ${PROJECT_SOURCE_DIR}/${curdir}/*)
		
		foreach(child ${children})
			if(IS_DIRECTORY ${PROJECT_SOURCE_DIR}/${curdir}/${child})
				GroupSources(${curdir}/${child})
			else() 
				string(REPLACE "/" "\\" groupname ${curdir})
				string(REPLACE "src" "Sources" groupname ${groupname})
				source_group(${groupname} FILES ${PROJECT_SOURCE_DIR}/${curdir}/${child})
			endif()
		endforeach()
	endmacro()
	
	# Run Macro
	GroupSources(src)
	GroupSources(include/jc3DTestSharedLibs)
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
	Taskflow::Taskflow
	imgui::imgui 

	)