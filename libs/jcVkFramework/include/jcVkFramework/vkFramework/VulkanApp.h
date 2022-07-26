#ifndef __VULKAN_APP_H__
#define __VULKAN_APP_H__

#define VK_NO_PROTOTYPES
#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <chrono>
#include <memory>
#include <limits>
#include <deque>

//#include "Utils.h"
//#include "UtilsMath.h"
//#include "UtilsVulkan.h"
#include <jcVkFramework/Utils.h>
#include <jcVkFramework/UtilsMath.h>
#include <jcVkFramework/UtilsVulkan.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

using glm::mat4;
using glm::vec4;
using glm::vec3;
using glm::vec2;

struct Resolution
{
	uint32_t width = 0;
	uint32_t height = 0;
};

GLFWwindow* initVulkanApp ( int width, int height, Resolution* outResolution = nullptr );

extern "C" bool drawVulkanAppFrame (VulkanRenderDevice & vkDev, const std::function<void (uint32_t)>&updateBuffersFunc, const std::function<void (VkCommandBuffer, uint32_t)>&composeFrameFunc);
bool drawFrame ( VulkanRenderDevice& vkDev, const std::function<void ( uint32_t )>& updateBuffersFunc, const std::function<void ( VkCommandBuffer, uint32_t )>& composeFrameFunc );

namespace jcvkfw {
	GLFWwindow* jcInitVulkanApp ( int width, int height, Resolution* outResolution = nullptr );
	bool jcDrawFrame ( VulkanRenderDevice& vkDev, const std::function<void ( uint32_t )>& updateBuffersFunc, const std::function<void ( VkCommandBuffer, uint32_t )>& composeFrameFunc );
}

#endif // __VULKAN_APP_H__