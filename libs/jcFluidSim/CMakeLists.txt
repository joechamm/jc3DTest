cmake_minimum_required(VERSION 2.8...3.24)

project(jcFluidSim
	VERSION 0.1.0
	LANGUAGES CXX
	DESCRIPTION "Fluid simulation library for graphics applications")
	
# Debugging cmake 
include(CMakePrintHelpers)

message(STATUS "Building ${PROJECT_NAME}")

# Include any of our cmake helpers
include(${CMAKE_SOURCE_DIR}/cmake/setup-helpers.cmake)

# use the Eigen library for sparse matrix
include_directories(${CMAKE_SOURCE_DIR}/extern/src/eigen/Eigen)

file(GLOB_RECURSE SRC_FILES LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.c??)
file(GLOB_RECURSE HEADER_FILES LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.h)

cmake_print_variables(CMAKE_CURRENT_SOURCE_DIR)

message(STATUS "Checking file globs...")

cmake_print_variables(SRC_FILES)
cmake_print_variables(HEADER_FILES)

SETUP_GROUPS("${HEADER_FILES}")
SETUP_GROUPS("${SRC_FILES}")

add_library(jcFluidSim)

# export all our symbols for now
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
# TODO: refine our exports as in Chapter 3 of CMake Best Practices
# set_property(TARGET jcFluidSim PROPERTY CXX_VISIBILITY_PRESET "hidden")
# set_property(TARGET jcFluidSim PROPERTY VISIBILITY_INLINES_HIDDEN TRUE)
# include(GenerateExportHeader)
# generate_export_header(jcFluidSim EXPORT_FILE_NAME export/jcFluidSim/jcFluidSim.hpp)
# target_include_directories(jcFluidSim PUBLIC "${CMAKE_CURRENT_BINARY_DIR} /export")

target_sources(jcFluidSim PRIVATE ${SRC_FILES} ${HEADER_FILES})

target_compile_features(jcFluidSim PUBLIC cxx_std_20)

target_include_directories(jcFluidSim 
  PRIVATE 
  ${CMAKE_CURRENT_SOURCE_DIR}/src 
  ${CMAKE_CURRENT_SOURCE_DIR}/include/jcFluidSim 
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(jcFluidSim 
#  PUBLIC 
#  glad
#  glfw 
#  assimp
)

#if(BUILD_WITH_EASY_PROFILER)
#	target_link_libraries(jcFluidSim PUBLIC easy_profiler)
#endif()

#if(BUILD_WITH_OPTICK)
#	target_link_libraries(jcFluidSim PUBLIC OptickCore)
#endif()

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
	GroupSources(include/jcFluidSim)
endif()