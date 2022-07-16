# CMakeLists.txt for jc3DCh04_GL02_FPS

cmake_minimum_required(VERSION 3.14)

project(jc3DCh04_GL02_FPS CXX C)

add_executable(jc3DCh04_GL02_FPS)

set_property(TARGET jc3DCh04_GL02_FPS PROPERTY FOLDER "apps")

target_compile_features(jc3DCh04_GL02_FPS PRIVATE cxx_std_20)

target_sources(jc3DCh04_GL02_FPS 
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/src/Bitmap.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/Camera.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/debug.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/GLShader.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/Utils.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/UtilsCubemap.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/UtilsMath.h
	PRIVATE 
	${CMAKE_CURRENT_SOURCE_DIR}/src/GLShader.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Utils.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/UtilsCubemap.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_include_directories(jc3DCh04_GL02_FPS PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(jc3DCh04_GL02_FPS PRIVATE 
	glad 
	glfw 
	assimp)