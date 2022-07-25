# CMakeLists.txt for jc3DCh05_GL02_Tessellation

cmake_minimum_required(VERSION 3.14)

project(jc3DCh05_GL02_Tessellation CXX C)

add_executable(jc3DCh05_GL02_Tessellation)

set_property(TARGET jc3DCh05_GL02_Tessellation PROPERTY FOLDER "apps")

target_compile_features(jc3DCh05_GL02_Tessellation PUBLIC cxx_std_20)

target_sources(jc3DCh05_GL02_Tessellation 
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/src/Utils.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/UtilsMath.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/Camera.h 
	${CMAKE_CURRENT_SOURCE_DIR}/src/debug.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/ResourceString.h
#	${CMAKE_CURRENT_SOURCE_DIR}/src/glFramework/GLFWApp.h 
	${CMAKE_CURRENT_SOURCE_DIR}/src/glFramework/GLShader.h 
	${CMAKE_CURRENT_SOURCE_DIR}/src/glFramework/UtilsGLImGui.h 
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_wrapper.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Utils.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/glFramework/GLShader.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

## imgui sources
#target_sources(jc3DCh05_GL02_Tessellation 
#	PUBLIC 
#	${CMAKE_CURRENT_SOURCE_DIR}/src/imconfig.h
#	${CMAKE_CURRENT_SOURCE_DIR}/src/imgui.h 
#	${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_internal.h
#	${CMAKE_CURRENT_SOURCE_DIR}/src/imstb_rectpack.h  
#	${CMAKE_CURRENT_SOURCE_DIR}/src/imstb_textedit.h 
#	${CMAKE_CURRENT_SOURCE_DIR}/src/imstb_truetype.h
#	PRIVATE 
#	${CMAKE_CURRENT_SOURCE_DIR}/src/imgui.cpp
#	${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_demo.cpp 
#	${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_draw.cpp 
#	${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_tables.cpp 
#	${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_widgets.cpp 
#	${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_wrapper.cpp
#)

target_include_directories(jc3DCh05_GL02_Tessellation PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/src/glFramework)

target_link_libraries(jc3DCh05_GL02_Tessellation PRIVATE
	glad
	glfw
	assimp
)
