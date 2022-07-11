cmake_minimum_required(VERSION 3.14)

project(jc3DCh02_ETC2Comp CXX C)

add_executable(jc3DCh02_ETC2Comp)

set_property(TARGET jc3DCh02_ETC2Comp PROPERTY FOLDER "apps")

target_compile_features(jc3DCh02_ETC2Comp PRIVATE cxx_std_20)

target_sources(jc3DCh02_ETC2Comp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_sources(jc3DCh02_ETC2Comp PUBLIC ${CMAKE_SOURCE_DIR}/extern/src/etc2comp/EtcTool/EtcFile.cpp)
target_sources(jc3DCh02_ETC2Comp PUBLIC ${CMAKE_SOURCE_DIR}/extern/src/etc2comp/EtcTool/EtcFileHeader.cpp)

target_include_directories(jc3DCh02_ETC2Comp PUBLIC ${CMAKE_SOURCE_DIR}/extern/src/etc2comp/EtcTool)
target_include_directories(jc3DCh02_ETC2Comp PUBLIC ${CMAKE_SOURCE_DIR}/extern/src/etc2comp/EtcLib/Etc)
target_include_directories(jc3DCh02_ETC2Comp PUBLIC ${CMAKE_SOURCE_DIR}/extern/src/etc2comp/EtcLib/EtcCodec)

#if(MSVC)
#	set_property(TARGET jc3DCh02_ex07 PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
#endif()

target_link_libraries(jc3DCh02_ETC2Comp PUBLIC  
	glad::glad 
	glfw
	glm::glm 
	EtcLib)