# CMakeLists.txt for jc3DCh05_GL03_MeshRenderer

cmake_minimum_required(VERSION 3.14)

project(jc3DCh05_GL03_MeshRenderer CXX C)

add_executable(jc3DCh05_GL03_MeshRenderer)

set_property(TARGET jc3DCh05_GL03_MeshRenderer PROPERTY FOLDER "apps")

target_compile_features(jc3DCh05_GL03_MeshRenderer PUBLIC cxx_std_20)

target_sources(jc3DCh05_GL03_MeshRenderer 
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/src/Utils.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/UtilsMath.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/Camera.h 
	${CMAKE_CURRENT_SOURCE_DIR}/src/debug.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/ResourceString.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/glFramework/GLFWApp.h 
	${CMAKE_CURRENT_SOURCE_DIR}/src/glFramework/GLShader.h 
	${CMAKE_CURRENT_SOURCE_DIR}/src/VtxData.h
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src/Utils.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/glFramework/GLShader.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/VtxData.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_include_directories(jc3DCh05_GL03_MeshRenderer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/src/glFramework)

target_link_libraries(jc3DCh05_GL03_MeshRenderer PRIVATE
	glad
	glfw
	assimp
)
