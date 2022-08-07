# CMakeLists.txt for jc3DCh08_GL01_ShadowMapping

cmake_minimum_required(VERSION 3.14)

project(jc3DCh08_GL01_ShadowMapping CXX C)

include_directories(${CMAKE_SOURCE_DIR}/libs/jcGLframework/include)
include_directories(${CMAKE_SOURCE_DIR}/extern/src/imgui)

add_executable(jc3DCh08_GL01_ShadowMapping)

set_property(TARGET jc3DCh08_GL01_ShadowMapping PROPERTY FOLDER "apps")

target_compile_features(jc3DCh08_GL01_ShadowMapping PUBLIC cxx_std_20)

target_sources(jc3DCh08_GL01_ShadowMapping 
	PUBLIC
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_include_directories(jc3DCh08_GL01_ShadowMapping PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_include_directories(jc3DCh08_GL01_ShadowMapping PRIVATE ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include/jcGLframework ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include/jcGLframework/scene ${CMAKE_SOURCE_DIR}/libs/jcGLframework/include/jcGLframework/glFramework)

target_link_libraries(jc3DCh08_GL01_ShadowMapping PRIVATE
	jcGLframework 
)

if(BUILD_WITH_EASY_PROFILER)
	target_link_libraries(jc3DCh08_GL01_ShadowMapping PRIVATE easy_profiler)
endif()