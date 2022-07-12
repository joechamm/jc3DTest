# CMakeLists.txt for jcVK01_GLSLang

cmake_minimum_required(VERSION 3.14)

project(jc3DCh03_jcVK01_GLSLang CXX C)

add_executable(jc3DCh03_jcVK01_GLSLang)

set_property(TARGET jc3DCh03_jcVK01_GLSLang PROPERTY FOLDER "apps")

target_compile_features(jc3DCh03_jcVK01_GLSLang PRIVATE cxx_std_20)

target_sources(jc3DCh03_jcVK01_GLSLang PRIVATE 
	${CMAKE_CURRENT_SOURCE_DIR}/src/Utils.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/UtilsVulkan.cpp 
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_include_directories(jc3DCh03_jcVK01_GLSLang PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

#if(MSVC)
#	set_property(TARGET jc3DCh03_jcVK01_GLSLang PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
#endif()

target_link_libraries(jc3DCh03_jcVK01_GLSLang PRIVATE 
	glfw
	volk::volk 
	volk::volk_headers
	glslang
	SPIRV
)