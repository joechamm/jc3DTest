# CMakeLists.txt for jc3DCh10_GL03_OITransparency

cmake_minimum_required(VERSION 3.14)

project(jc3DCh10_GL03_OITransparency CXX C)

include_directories(${CMAKE_SOURCE_DIR}/libs/jcGLframework/include)
include_directories(${CMAKE_SOURCE_DIR}/extern/src/imgui)

add_executable(jc3DCh10_GL03_OITransparency)

set_property(TARGET jc3DCh10_GL03_OITransparency PROPERTY FOLDER "apps")

target_compile_features(jc3DCh10_GL03_OITransparency PUBLIC cxx_std_20)

target_sources(jc3DCh10_GL03_OITransparency 
	PUBLIC
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_include_directories(jc3DCh10_GL03_OITransparency PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_include_directories(jc3DCh10_GL03_OITransparency PRIVATE ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include/jcGLframework ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include/jcGLframework/scene ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include/jcGLframework/glFramework)

target_link_libraries(jc3DCh10_GL03_OITransparency PRIVATE
	jcGLframework 
)

if(BUILD_WITH_EASY_PROFILER)
	target_link_libraries(jc3DCh10_GL03_OITransparency PRIVATE easy_profiler)
endif()