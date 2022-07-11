# CMakeLists.txt for jcGL01_APIWrapping

cmake_minimum_required(VERSION 3.14)

project(jc3DCh03_GL01ApiWrapping CXX C)

if(WIN32)
	set(PYTHON_EXECUTABLE "python")
else()
	set(PYTHON_EXECUTABLE "python3")
endif()

set(py_cmd "src/GetGLAPI.py")
set(result_filename "src/GLAPITrace.h")

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/GLAPITrace.h")
	execute_process(
		COMMAND ${PYTHON_EXECUTABLE} ${py_cmd}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
		RESULT_VARIABLE py_result
	)
	if (NOT (${py_result} EQUAL 0))
		file(WRITE ${result_filename} ${py_result})
	endif()
endif()

add_executable(jc3DCh03_GL01ApiWrapping)

set_property(TARGET jc3DCh03_GL01ApiWrapping PROPERTY FOLDER "apps")

target_compile_features(jc3DCh03_GL01ApiWrapping PRIVATE cxx_std_20)

target_sources(jc3DCh03_GL01ApiWrapping PRIVATE 
	${CMAKE_CURRENT_SOURCE_DIR}/src/GLAPITrace.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp	
)

target_include_directories(jc3DCh03_GL01ApiWrapping PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(jc3DCh03_GL01ApiWrapping PRIVATE
	glfw)