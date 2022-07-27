#include <jcVkFramework/vkFramework/VulkanApp.h>

Resolution detectResolution ( int width, int height )
{
	GLFWmonitor* monitor = glfwGetPrimaryMonitor ();
	if ( glfwGetError ( nullptr ) ) exit ( EXIT_FAILURE );
	const GLFWvidmode* info = glfwGetVideoMode ( monitor );
	const uint32_t W = width >= 0 ? width : (uint32_t)(info->width * width / -100);
	const uint32_t H = height >= 0 ? height : (uint32_t)(info->height * height / -100);
	return Resolution{
		.width = W,
		.height = H
	};
}

GLFWwindow* initVulkanApp ( int width, int height, Resolution* outResolution )
{
	glslang_initialize_process ();
	volkInitialize ();

	if ( !glfwInit () || !glfwVulkanSupported () )
	{
#ifndef NDEBUG
		printf ( "initVulkan failed\n" );
#endif
		exit ( EXIT_FAILURE );
	}

	glfwWindowHint ( GLFW_CLIENT_API, GLFW_NO_API );
	glfwWindowHint ( GLFW_RESIZABLE, GLFW_FALSE );

	if ( outResolution )
	{
		*outResolution = detectResolution ( width, height );
		width = outResolution->width;
		height = outResolution->height;
	}

	GLFWwindow* result = glfwCreateWindow ( width, height, "VulkanApp", nullptr, nullptr );
	if ( !result )
	{
#ifndef NDEBUG
		printf ( "VulkanApp: glfwCreateWindow failed\n" );
#endif
		glfwTerminate ();
		exit ( EXIT_FAILURE );
	}

	return result;
}

bool drawFrame ( VulkanRenderDevice& vkDev, const std::function<void ( uint32_t )>& updateBuffersFunc, const std::function<void ( VkCommandBuffer, uint32_t )>& composeFrameFunc )
{
	/*
	*	Before we can render anything on the screen, we should acquire a framebuffer image from the swapchain. When the next image in the swapchain is not ready to be rendered, we return false.
	*	This is not a fatal error but an indication that no frame has been rendered yet. The calling code decides what to do with the result.
	*	One example of such a reaction can be skipping the frames-per-second (FPS) counter update:
	*/
	uint32_t imageIndex = 0;
	VkResult result = vkAcquireNextImageKHR (/*VkDevice*/ vkDev.device, /*VkSwapchain*/ vkDev.swapchain,/*uint64_t timeout*/ 0,/*VkSemaphore*/ vkDev.semaphore,/*VkFence*/ VK_NULL_HANDLE, /*uint32_t *pImageIndex*/ &imageIndex );
	if ( result != VK_SUCCESS )  return false;

	/* reset the global command pool to allow the filling of new command buffers */
	VK_CHECK ( vkResetCommandPool ( vkDev.device, vkDev.commandPool, 0 ) );

	/*
	*	The updateBuffersFunc() callback is invoked to update all the internal buffers for different renderers.
	*	Revisit the Organizing Vulkan frame rendering code recipe from Chapter 4, Adding User Interaction and Productivity Tools for a discussion of frame composition.
	*	This can be done in a more effective way—for example, by using a dedicated transfer queue and without waiting for all the GPU transfers to complete.
	*	For the purpose of this book, we deliberately choose code simplicity over performance. A command buffer, corresponding to the selected swapchain image, is acquired
	*/

	updateBuffersFunc ( imageIndex );
	VkCommandBuffer commandBuffer = vkDev.commandBuffers[imageIndex];
	const VkCommandBufferBeginInfo bi = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
		.pInheritanceInfo = nullptr
	};

	VK_CHECK ( vkBeginCommandBuffer ( commandBuffer, &bi ) );

	/*
	*	After we have started recording to the command buffer, the composeFrameFunc() callback is invoked to write the command buffer's contents from different renderers.
	*	There is a large potential for optimizations here because Vulkan provides a primary-secondary command buffer separation, which can be used to record secondary buffers from multiple central processing unit (CPU) threads.
	*	Once all the renderers have contributed to the command buffer, we stop recording
	*/
	composeFrameFunc ( commandBuffer, imageIndex );
	VK_CHECK ( vkEndCommandBuffer ( commandBuffer ) );

	/* Next comes the submission of the recorded command buffer to a GPU graphics queue. */
	const VkPipelineStageFlags waitStages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};

	const VkSubmitInfo si = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vkDev.semaphore,
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &vkDev.commandBuffers[imageIndex],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &vkDev.renderSemaphore
	};

	VK_CHECK ( vkQueueSubmit ( vkDev.graphicsQueue, 1, &si, nullptr ) );

	/* After submitting the command buffer to the graphics queue, the swapchain is presented to the screen */
	const VkPresentInfoKHR pi = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vkDev.renderSemaphore,
		.swapchainCount = 1,
		.pSwapchains = &vkDev.swapchain,
		.pImageIndices = &imageIndex
	};

	VK_CHECK ( vkQueuePresentKHR ( vkDev.graphicsQueue, &pi ) );

	/* The final call to vkDeviceWaitIdle() ensures we ahve no frame tearing */
	VK_CHECK ( vkDeviceWaitIdle ( vkDev.device ) );

	return true;
}


extern "C" bool drawVulkanAppFrame (VulkanRenderDevice & vkDev, const std::function<void (uint32_t)>&updateBuffersFunc, const std::function<void (VkCommandBuffer, uint32_t)>&composeFrameFunc)
{
	/*	
	*	Before we can render anything on the screen, we should acquire a framebuffer image from the swapchain. When the next image in the swapchain is not ready to be rendered, we return false. 
	*	This is not a fatal error but an indication that no frame has been rendered yet. The calling code decides what to do with the result. 
	*	One example of such a reaction can be skipping the frames-per-second (FPS) counter update:
	*/
	uint32_t imageIndex = 0;
	VkResult result = vkAcquireNextImageKHR (/*VkDevice*/ vkDev.device, /*VkSwapchain*/ vkDev.swapchain,/*uint64_t timeout*/ 0,/*VkSemaphore*/ vkDev.semaphore,/*VkFence*/ VK_NULL_HANDLE, /*uint32_t *pImageIndex*/ &imageIndex);
	if ( result != VK_SUCCESS )  return false;

	/* reset the global command pool to allow the filling of new command buffers */
	VK_CHECK ( vkResetCommandPool ( vkDev.device, vkDev.commandPool, 0 ) );

	/*
	*	The updateBuffersFunc() callback is invoked to update all the internal buffers for different renderers. 
	*	Revisit the Organizing Vulkan frame rendering code recipe from Chapter 4, Adding User Interaction and Productivity Tools for a discussion of frame composition. 
	*	This can be done in a more effective way—for example, by using a dedicated transfer queue and without waiting for all the GPU transfers to complete. 
	*	For the purpose of this book, we deliberately choose code simplicity over performance. A command buffer, corresponding to the selected swapchain image, is acquired
	*/

	updateBuffersFunc ( imageIndex );
	VkCommandBuffer commandBuffer = vkDev.commandBuffers[imageIndex];
	const VkCommandBufferBeginInfo bi = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
		.pInheritanceInfo = nullptr
	};

	VK_CHECK ( vkBeginCommandBuffer ( commandBuffer, &bi ) );

	/*
	*	After we have started recording to the command buffer, the composeFrameFunc() callback is invoked to write the command buffer's contents from different renderers. 
	*	There is a large potential for optimizations here because Vulkan provides a primary-secondary command buffer separation, which can be used to record secondary buffers from multiple central processing unit (CPU) threads. 
	*	Once all the renderers have contributed to the command buffer, we stop recording
	*/
	composeFrameFunc ( commandBuffer, imageIndex );
	VK_CHECK ( vkEndCommandBuffer ( commandBuffer ) );

	/* Next comes the submission of the recorded command buffer to a GPU graphics queue. */
	const VkPipelineStageFlags waitStages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};

	const VkSubmitInfo si = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vkDev.semaphore,
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &vkDev.commandBuffers[imageIndex],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &vkDev.renderSemaphore
	};

	VK_CHECK ( vkQueueSubmit ( vkDev.graphicsQueue, 1, &si, nullptr ) );

	/* After submitting the command buffer to the graphics queue, the swapchain is presented to the screen */
	const VkPresentInfoKHR pi = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vkDev.renderSemaphore,
		.swapchainCount = 1,
		.pSwapchains = &vkDev.swapchain,
		.pImageIndices = &imageIndex
	};

	VK_CHECK ( vkQueuePresentKHR ( vkDev.graphicsQueue, &pi ) );

	/* The final call to vkDeviceWaitIdle() ensures we ahve no frame tearing */
	VK_CHECK ( vkDeviceWaitIdle ( vkDev.device ) );

	return true;
}

GLFWwindow* jcvkfw::jcInitVulkanApp ( int width, int height, Resolution* outResolution )
{
	glslang_initialize_process ();
	volkInitialize ();

	if ( !glfwInit () || !glfwVulkanSupported () )
	{
#ifndef NDEBUG
		printf ( "initVulkan failed\n" );
#endif
		exit ( EXIT_FAILURE );
	}

	glfwWindowHint ( GLFW_CLIENT_API, GLFW_NO_API );
	glfwWindowHint ( GLFW_RESIZABLE, GLFW_FALSE );

	if ( outResolution )
	{
		*outResolution = detectResolution ( width, height );
		width = outResolution->width;
		height = outResolution->height;
	}

	GLFWwindow* result = glfwCreateWindow ( width, height, "VulkanApp", nullptr, nullptr );
	if ( !result )
	{
#ifndef NDEBUG
		printf ( "VulkanApp: glfwCreateWindow failed\n" );
#endif
		glfwTerminate ();
		exit ( EXIT_FAILURE );
	}

	return result;
}

bool jcvkfw::jcDrawFrame ( VulkanRenderDevice& vkDev, const std::function<void ( uint32_t )>& updateBuffersFunc, const std::function<void ( VkCommandBuffer, uint32_t )>& composeFrameFunc )
{
	/*
	*	Before we can render anything on the screen, we should acquire a framebuffer image from the swapchain. When the next image in the swapchain is not ready to be rendered, we return false.
	*	This is not a fatal error but an indication that no frame has been rendered yet. The calling code decides what to do with the result.
	*	One example of such a reaction can be skipping the frames-per-second (FPS) counter update:
	*/
	uint32_t imageIndex = 0;
	VkResult result = vkAcquireNextImageKHR (/*VkDevice*/ vkDev.device, /*VkSwapchain*/ vkDev.swapchain,/*uint64_t timeout*/ 0,/*VkSemaphore*/ vkDev.semaphore,/*VkFence*/ VK_NULL_HANDLE, /*uint32_t *pImageIndex*/ &imageIndex );
	if ( result != VK_SUCCESS )  return false;

	/* reset the global command pool to allow the filling of new command buffers */
	VK_CHECK ( vkResetCommandPool ( vkDev.device, vkDev.commandPool, 0 ) );

	/*
	*	The updateBuffersFunc() callback is invoked to update all the internal buffers for different renderers.
	*	Revisit the Organizing Vulkan frame rendering code recipe from Chapter 4, Adding User Interaction and Productivity Tools for a discussion of frame composition.
	*	This can be done in a more effective way—for example, by using a dedicated transfer queue and without waiting for all the GPU transfers to complete.
	*	For the purpose of this book, we deliberately choose code simplicity over performance. A command buffer, corresponding to the selected swapchain image, is acquired
	*/

	updateBuffersFunc ( imageIndex );
	VkCommandBuffer commandBuffer = vkDev.commandBuffers[imageIndex];
	const VkCommandBufferBeginInfo bi = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
		.pInheritanceInfo = nullptr
	};

	VK_CHECK ( vkBeginCommandBuffer ( commandBuffer, &bi ) );

	/*
	*	After we have started recording to the command buffer, the composeFrameFunc() callback is invoked to write the command buffer's contents from different renderers.
	*	There is a large potential for optimizations here because Vulkan provides a primary-secondary command buffer separation, which can be used to record secondary buffers from multiple central processing unit (CPU) threads.
	*	Once all the renderers have contributed to the command buffer, we stop recording
	*/
	composeFrameFunc ( commandBuffer, imageIndex );
	VK_CHECK ( vkEndCommandBuffer ( commandBuffer ) );

	/* Next comes the submission of the recorded command buffer to a GPU graphics queue. */
	const VkPipelineStageFlags waitStages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};

	const VkSubmitInfo si = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vkDev.semaphore,
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &vkDev.commandBuffers[imageIndex],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &vkDev.renderSemaphore
	};

	VK_CHECK ( vkQueueSubmit ( vkDev.graphicsQueue, 1, &si, nullptr ) );

	/* After submitting the command buffer to the graphics queue, the swapchain is presented to the screen */
	const VkPresentInfoKHR pi = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vkDev.renderSemaphore,
		.swapchainCount = 1,
		.pSwapchains = &vkDev.swapchain,
		.pImageIndices = &imageIndex
	};

	VK_CHECK ( vkQueuePresentKHR ( vkDev.graphicsQueue, &pi ) );

	/* The final call to vkDeviceWaitIdle() ensures we ahve no frame tearing */
	VK_CHECK ( vkDeviceWaitIdle ( vkDev.device ) );

	return true;
}
