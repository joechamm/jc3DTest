#include "Utils.h"
#include "UtilsVulkan.h"

#include <glslang/Include/ResourceLimits.h>

#include <cstdio>
#include <cstdlib>

glslang_stage_t glslangShaderStageFromFileName ( const char* fileName )
{
	if ( endsWith ( fileName, ".vert" ) )
		return GLSLANG_STAGE_VERTEX;
	if ( endsWith ( fileName, ".frag" ) )
		return GLSLANG_STAGE_FRAGMENT;
	if ( endsWith ( fileName, ".geom" ) )
		return GLSLANG_STAGE_GEOMETRY;
	if ( endsWith ( fileName, ".comp" ) )
		return GLSLANG_STAGE_COMPUTE;
	if ( endsWith ( fileName, ".tesc" ) )
		return GLSLANG_STAGE_TESSCONTROL;
	if ( endsWith ( fileName, ".tese" ) )
		return GLSLANG_STAGE_TESSEVALUATION;

	return GLSLANG_STAGE_VERTEX;
}

namespace glslang
{
	static TBuiltInResource InitResources ()
	{
		TBuiltInResource Resources;

		Resources.maxLights = 32;
		Resources.maxClipPlanes = 6;
		Resources.maxTextureUnits = 32;
		Resources.maxTextureCoords = 32;
		Resources.maxVertexAttribs = 64;
		Resources.maxVertexUniformComponents = 4096;
		Resources.maxVaryingFloats = 64;
		Resources.maxVertexTextureImageUnits = 32;
		Resources.maxCombinedTextureImageUnits = 80;
		Resources.maxTextureImageUnits = 32;
		Resources.maxFragmentUniformComponents = 4096;
		Resources.maxDrawBuffers = 32;
		Resources.maxVertexUniformVectors = 128;
		Resources.maxVaryingVectors = 8;
		Resources.maxFragmentUniformVectors = 16;
		Resources.maxVertexOutputVectors = 16;
		Resources.maxFragmentInputVectors = 15;
		Resources.minProgramTexelOffset = -8;
		Resources.maxProgramTexelOffset = 7;
		Resources.maxClipDistances = 8;
		Resources.maxComputeWorkGroupCountX = 65535;
		Resources.maxComputeWorkGroupCountY = 65535;
		Resources.maxComputeWorkGroupCountZ = 65535;
		Resources.maxComputeWorkGroupSizeX = 1024;
		Resources.maxComputeWorkGroupSizeY = 1024;
		Resources.maxComputeWorkGroupSizeZ = 64;
		Resources.maxComputeUniformComponents = 1024;
		Resources.maxComputeTextureImageUnits = 16;
		Resources.maxComputeImageUniforms = 8;
		Resources.maxComputeAtomicCounters = 8;
		Resources.maxComputeAtomicCounterBuffers = 1;
		Resources.maxVaryingComponents = 60;
		Resources.maxVertexOutputComponents = 64;
		Resources.maxGeometryInputComponents = 64;
		Resources.maxGeometryOutputComponents = 128;
		Resources.maxFragmentInputComponents = 128;
		Resources.maxImageUnits = 8;
		Resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
		Resources.maxCombinedShaderOutputResources = 8;
		Resources.maxImageSamples = 0;
		Resources.maxVertexImageUniforms = 0;
		Resources.maxTessControlImageUniforms = 0;
		Resources.maxTessEvaluationImageUniforms = 0;
		Resources.maxGeometryImageUniforms = 0;
		Resources.maxFragmentImageUniforms = 8;
		Resources.maxCombinedImageUniforms = 8;
		Resources.maxGeometryTextureImageUnits = 16;
		Resources.maxGeometryOutputVertices = 256;
		Resources.maxGeometryTotalOutputComponents = 1024;
		Resources.maxGeometryUniformComponents = 1024;
		Resources.maxGeometryVaryingComponents = 64;
		Resources.maxTessControlInputComponents = 128;
		Resources.maxTessControlOutputComponents = 128;
		Resources.maxTessControlTextureImageUnits = 16;
		Resources.maxTessControlUniformComponents = 1024;
		Resources.maxTessControlTotalOutputComponents = 4096;
		Resources.maxTessEvaluationInputComponents = 128;
		Resources.maxTessEvaluationOutputComponents = 128;
		Resources.maxTessEvaluationTextureImageUnits = 16;
		Resources.maxTessEvaluationUniformComponents = 1024;
		Resources.maxTessPatchComponents = 120;
		Resources.maxPatchVertices = 32;
		Resources.maxTessGenLevel = 64;
		Resources.maxViewports = 16;
		Resources.maxVertexAtomicCounters = 0;
		Resources.maxTessControlAtomicCounters = 0;
		Resources.maxTessEvaluationAtomicCounters = 0;
		Resources.maxGeometryAtomicCounters = 0;
		Resources.maxFragmentAtomicCounters = 8;
		Resources.maxCombinedAtomicCounters = 8;
		Resources.maxAtomicCounterBindings = 1;
		Resources.maxVertexAtomicCounterBuffers = 0;
		Resources.maxTessControlAtomicCounterBuffers = 0;
		Resources.maxTessEvaluationAtomicCounterBuffers = 0;
		Resources.maxGeometryAtomicCounterBuffers = 0;
		Resources.maxFragmentAtomicCounterBuffers = 1;
		Resources.maxCombinedAtomicCounterBuffers = 1;
		Resources.maxAtomicCounterBufferSize = 16384;
		Resources.maxTransformFeedbackBuffers = 4;
		Resources.maxTransformFeedbackInterleavedComponents = 64;
		Resources.maxCullDistances = 8;
		Resources.maxCombinedClipAndCullDistances = 8;
		Resources.maxSamples = 4;
		Resources.maxMeshOutputVerticesNV = 256;
		Resources.maxMeshOutputPrimitivesNV = 512;
		Resources.maxMeshWorkGroupSizeX_NV = 32;
		Resources.maxMeshWorkGroupSizeY_NV = 1;
		Resources.maxMeshWorkGroupSizeZ_NV = 1;
		Resources.maxTaskWorkGroupSizeX_NV = 32;
		Resources.maxTaskWorkGroupSizeY_NV = 1;
		Resources.maxTaskWorkGroupSizeZ_NV = 1;
		Resources.maxMeshViewCountNV = 4;

		Resources.limits.nonInductiveForLoops = 1;
		Resources.limits.whileLoops = 1;
		Resources.limits.doWhileLoops = 1;
		Resources.limits.generalUniformIndexing = 1;
		Resources.limits.generalAttributeMatrixVectorIndexing = 1;
		Resources.limits.generalVaryingIndexing = 1;
		Resources.limits.generalSamplerIndexing = 1;
		Resources.limits.generalVariableIndexing = 1;
		Resources.limits.generalConstantMatrixVectorIndexing = 1;

		return Resources;
	}
}

static_assert(sizeof ( TBuiltInResource ) == sizeof ( glslang_resource_t ));

static size_t compileShader ( glslang_stage_t stage, const char* shaderSource, ShaderModule& shaderModule )
{
	static TBuiltInResource DefaultTBuildInResources = glslang::InitResources ();

	const glslang_input_t input = {
		.language = GLSLANG_SOURCE_GLSL,
		.stage = stage,
		.client = GLSLANG_CLIENT_VULKAN,
		.client_version = GLSLANG_TARGET_VULKAN_1_1,
		.target_language = GLSLANG_TARGET_SPV,
		.target_language_version = GLSLANG_TARGET_SPV_1_3,
		.code = shaderSource,
		.default_version = 100,
		.default_profile = GLSLANG_NO_PROFILE,
		.force_default_version_and_profile = false,
		.forward_compatible = false,
		.messages = GLSLANG_MSG_DEFAULT_BIT,
		.resource = (const glslang_resource_t*)&DefaultTBuildInResources,
	};

	glslang_shader_t* shd = glslang_shader_create ( &input );

	if ( !glslang_shader_preprocess ( shd, &input ) )
	{
		fprintf ( stderr, "GLSL preprocessing failed\n" );
		fprintf ( stderr, "\n%s", glslang_shader_get_info_log ( shd ) );
		fprintf ( stderr, "\n%s", glslang_shader_get_info_debug_log ( shd ) );
		fprintf ( stderr, "code:\n%s", input.code );
		return 0;
	}

	if ( !glslang_shader_parse ( shd, &input ) )
	{
		fprintf ( stderr, "GLSL parsing failed\n" );
		fprintf ( stderr, "\n%s", glslang_shader_get_info_log ( shd ) );
		fprintf ( stderr, "\n%s", glslang_shader_get_info_debug_log ( shd ) );
		fprintf ( stderr, "%s", glslang_shader_get_preprocessed_code ( shd ) );
		return 0;
	}

	glslang_program_t* prog = glslang_program_create ();
	glslang_program_add_shader ( prog, shd );
	int msgs = GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT;
	if ( !glslang_program_link ( prog, msgs ) )
	{
		fprintf ( stderr, "GLSL linking failed\n" );
		fprintf ( stderr, "\n%s", glslang_program_get_info_log ( prog ) );
		fprintf ( stderr, "\n%s", glslang_program_get_info_debug_log ( prog ) );
		return 0;
	}

	glslang_program_SPIRV_generate ( prog, stage );
	shaderModule.SPIRV.resize ( glslang_program_SPIRV_get_size ( prog ) );
	glslang_program_SPIRV_get ( prog, shaderModule.SPIRV.data () );
	const char* spirv_messages = glslang_program_SPIRV_get_messages ( prog );
	if ( spirv_messages )
		fprintf ( stderr, "%s", spirv_messages );

	glslang_program_delete ( prog );
	glslang_shader_delete ( shd );
	return shaderModule.SPIRV.size ();
}

size_t compileShaderFile ( const char* file, ShaderModule& shaderModule )
{
	if ( auto shaderSource = readShaderFile ( file ); !shaderSource.empty () )
		return compileShader ( glslangShaderStageFromFileName ( file ), shaderSource.c_str (), shaderModule );

	return 0;		
}