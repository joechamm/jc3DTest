cmake_minimum_required(VERSION 3.14)

project(jcCh02_ex07 CXX C)

find_package(glad CONFIG REQUIRED)
if(glad_FOUND)
	message(STATUS "Found GLAD")
else()
	message(STATUS "GLAD NOT FOUND!")
endif()
find_package(glfw3 CONFIG REQUIRED)
if(glfw3_FOUND)
	message(STATUS "Found GLFW3")
else()
	message(STATUS "GLFW3 NOT FOUND!")
endif()
find_package(assimp CONFIG REQUIRED)
if(assimp_FOUND)
	message(STATUS "Found ASSIMP")
else()
	message(STATUS "ASSIMP NOT FOUND!")
endif()
find_package(glm CONFIG REQUIRED)
if(glm_FOUND)
	message(STATUS "Found GLM")
else()
	message(STATUS "GLM NOT FOUND!")
endif()

add_executable(jcCh02_ex07)

#set_property(TARGET jcCh02_ex07 PROPERTY CXX_STANDARD 20)
#set_property(TARGET jcCh02_ex07 PROPERTY CXX_STANDARD_REQUIRED ON)

set_property(TARGET jcCh02_ex07 PROPERTY FOLDER "apps/jcCh02_ex07")

target_compile_features(jcCh02_ex07 PRIVATE cxx_std_20)

target_sources(jcCh02_ex07 PRIVATE "src/main.cpp")

if(MSVC)
	set_property(TARGET jcCh02_ex07 PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()

target_link_libraries(jcCh02_ex07 PRIVATE 
	glad::glad 
	glfw
	glm::glm 
	assimp::assimp)