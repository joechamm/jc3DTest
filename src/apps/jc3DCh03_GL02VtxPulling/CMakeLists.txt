# CMakeLists.txt for jcGL02_VtxPulling

cmake_minimum_required(VERSION 3.14)

project(jc3DCh03_GL02VtxPulling CXX C)

add_executable(jc3DCh03_GL02VtxPulling)

set_property(TARGET jc3DCh03_GL02VtxPulling PROPERTY FOLDER "apps")

target_compile_features(jc3DCh03_GL02VtxPulling PRIVATE cxx_std_20)

target_sources(jc3DCh03_GL02VtxPulling PRIVATE 
	${CMAKE_CURRENT_SOURCE_DIR}/src/GLShader.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Utils.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
	
target_include_directories(jc3DCh03_GL02VtxPulling PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src)
	
target_link_libraries(jc3DCh03_GL02VtxPulling PRIVATE
	glad::glad 
	glfw
	glm::glm 
	assimp::assimp)