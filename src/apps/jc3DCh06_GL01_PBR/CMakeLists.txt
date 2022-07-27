# CMakeLists.txt for jc3DCh06_GL01_PBR

cmake_minimum_required(VERSION 3.14)

project(jc3DCh06_GL01_PBR CXX C)

include_directories(${CMAKE_SOURCE_DIR}/libs/jcGLframework/include)

add_executable(jc3DCh06_GL01_PBR)

set_property(TARGET jc3DCh06_GL01_PBR PROPERTY FOLDER "apps")

target_compile_features(jc3DCh06_GL01_PBR PUBLIC cxx_std_20)

target_sources(jc3DCh06_GL01_PBR 
	PUBLIC
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_include_directories(jc3DCh06_GL01_PBR PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_include_directories(jc3DCh06_GL01_PBR PRIVATE ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include/jcGLframework ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include/jcGLframework/scene ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include/jcGLframework/glFramework)

target_link_libraries(jc3DCh06_GL01_PBR PRIVATE
	jcGLframework 
)

if(BUILD_WITH_EASY_PROFILER)
	target_link_libraries(jc3DCh06_GL01_PBR PRIVATE easy_profiler)
endif()