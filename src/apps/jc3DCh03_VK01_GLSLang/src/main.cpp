#define VK_NO_PROTOTYPES
#define GLFW_INCLUDE_VULKAN

#include <cstdio>
#include <cstdlib>
#include <vector>

#include "apps/jc3DCh03_VK01_GLSLang/Utils.h"	
#include "apps/jc3DCh03_VK01_GLSLang/UtilsVulkan.h"

#include <iostream>
#include <string>

#include <helpers/RootDir.h>

using std::string;

void saveSPIRVBinaryFile ( const char* filename, unsigned int* code, size_t size )
{
	FILE* f = fopen ( filename, "w" );

	if ( !f )
		return;

	fwrite ( code, sizeof ( uint32_t ), size, f );
	fclose ( f );
}

void testShaderCompilation ( const char* sourceFilename, const char* destFilename )
{
	ShaderModule shaderModule;

	if ( compileShaderFile ( sourceFilename, shaderModule ) < 1 )
		return;

	saveSPIRVBinaryFile ( destFilename, shaderModule.SPIRV.data (), shaderModule.SPIRV.size () );
}

int main ()
{
	glslang_initialize_process ();

	testShaderCompilation ( string(ROOT_DIR).append("assets/shaders/VK01.vert").c_str (), "VK01.vert.bin");
	testShaderCompilation ( string ( ROOT_DIR ).append ( "assets/shaders/VK01.frag" ).c_str (), "VK01.frag.bin" );

	glslang_finalize_process ();

	return 0;
}