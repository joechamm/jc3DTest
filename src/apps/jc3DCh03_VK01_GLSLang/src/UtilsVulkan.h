#pragma once

#include <vector>

#define VK_NO_PROTOTYPES
#include <Volk/volk.h>

#include <glslang/Include/glslang_c_interface.h>

struct ShaderModule final {
	std::vector<unsigned int> SPIRV;
	VkShaderModule shaderModule;
};

VkResult createShaderModule ( VkDevice device, ShaderModule* shader, const char* fileName );

size_t compileShaderFile ( const char* file, ShaderModule& shaderModule );

