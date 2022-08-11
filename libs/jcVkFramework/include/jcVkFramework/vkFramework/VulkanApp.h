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

#include <imgui/imgui.h>

#include <jcCommonFramework/Camera.h>
#include <jcCommonFramework/Utils.h>
#include <jcCommonFramework/UtilsMath.h>
#include <jcVkFramework/UtilsVulkan.h>
#include <jcCommonFramework/UtilsFPS.h>

#include <jcVkFramework/vkFramework/VulkanResources.h>

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
bool drawFrame ( VulkanRenderDevice& vkDev, const std::function<void ( uint32_t )>& updateBuffersFunc, const std::function<void ( VkCommandBuffer, uint32_t )>& composeFrameFunc );

struct Renderer;

struct RenderItem
{
	Renderer& renderer_;
	bool enabled_ = true;
	bool useDepth_ = true;
	explicit RenderItem ( Renderer& r, bool useDepth = true ) : renderer_ ( r ), useDepth_ ( useDepth )
	{}
};

struct VulkanRenderContext
{
	VulkanInstance vk_;
	VulkanRenderDevice vkDev_;
	VulkanContextCreator ctxCreator_;
	VulkanResources resources_;

	std::vector<RenderItem> onScreenRenderers_;
	
	VulkanTexture depthTexture_;
	
	RenderPass screenRenderPass_;
	RenderPass screenRenderPass_NoDepth_;
	RenderPass clearRenderPass_;
	RenderPass finalRenderPass_;
	
	std::vector<VkFramebuffer> swapchainFramebuffers_;
	std::vector<VkFramebuffer> swapchainFramebuffers_NoDepth_;
	
	VulkanRenderContext(void* window, uint32_t screenWidth, uint32_t screenHeight, const VulkanContextFeatures& ctxFeatures = VulkanContextFeatures()) 
		: ctxCreator_(vk_, vkDev_, window, screenWidth, screenHeight, ctxFeatures)
		, resources_(vkDev_)

		, depthTexture_(resources_.addDepthTexture(vkDev_.framebufferWidth, vkDev_.framebufferHeight, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL))

		, screenRenderPass_(resources_.addFullScreenPass())
		, screenRenderPass_NoDepth_(resources_.addFullScreenPass(false))

		, finalRenderPass_(resources_.addFullScreenPass(true, RenderPassCreateInfo { .clearColor_ = false, .clearDepth_ = false, .flags_ = eRenderPassBit_Last }))
		, clearRenderPass_(resources_.addFullScreenPass(true, RenderPassCreateInfo { .clearColor_ = true, .clearDepth_ = true, .flags_ = eRenderPassBit_First }))
		
		, swapchainFramebuffers_(resources_.addFramebuffers(screenRenderPass_.handle_, depthTexture_.image.imageView))
		, swapchainFramebuffers_NoDepth_(resources_.addFramebuffers(screenRenderPass_NoDepth_.handle_))
	{
	}

	void updateBuffers ( uint32_t imageIndex );
	void beginRenderPass ( VkCommandBuffer cmdBuffer, VkRenderPass pass, size_t currentImage, const VkRect2D area, VkFramebuffer fb = VK_NULL_HANDLE, uint32_t clearValueCount = 0, const VkClearValue* clearValues = nullptr );
	void composeFrame(VkCommandBuffer commandBuffer, uint32_t currentImage);

	inline PipelineInfo pipelineParametersForOutputs ( const std::vector<VulkanTexture>& outputs ) const
	{
		return PipelineInfo {
			.width_ = outputs.empty () ? vkDev_.framebufferWidth : outputs [ 0 ].width,
			.height_ = outputs.empty () ? vkDev_.framebufferHeight : outputs [ 0 ].height,
			.useBlending_ = false
		};
	}
};

struct VulkanApp
{
protected:
	struct MouseState
	{
		glm::vec2 pos = glm::vec2 ( 0.0f );
		bool leftButtonPressed = false;
	} mouseState_;
	
	Resolution resolution_;
	GLFWwindow* window_ = nullptr;
	VulkanRenderContext ctx_;
	std::vector<RenderItem>& onScreenRenderers_;
	FramesPerSecondCounter fpsCounter_;
	
public:
	VulkanApp ( int screenWidth, int screenHeight, const VulkanContextFeatures& ctxFeatures = VulkanContextFeatures() )
		: window_ ( initVulkanApp ( screenWidth, screenHeight, &resolution_ ) )
		, ctx_ ( window_, resolution_.width, resolution_.height, ctxFeatures )
		, onScreenRenderers_ ( ctx_.onScreenRenderers_ )
	{
		glfwSetWindowUserPointer ( window_, this );
		assignCallbacks ();
	}

	~VulkanApp ()
	{
		glslang_finalize_process ();
		glfwTerminate ();
	}

	virtual void drawUI ()
	{}

	virtual void draw3D () = 0;
	virtual void update ( float dt ) = 0;

	void mainLoop ();

	inline bool shouldHandleMouse () const
	{
		return !ImGui::GetIO ().WantCaptureMouse;
	}

	virtual void handleKey ( int key, bool pressed ) = 0;
	
	virtual void handleMouseButton ( int button, bool pressed )
	{
		if ( button == GLFW_MOUSE_BUTTON_LEFT )
			mouseState_.leftButtonPressed = pressed;
	}

	virtual void handleMouseMove ( float x, float y )
	{
		mouseState_.pos = glm::vec2 ( x, y );
	}

	inline float getFPS() const { return fpsCounter_.getFPS(); }
	
private:
	void assignCallbacks ();
	void updateBuffers ( uint32_t imageIndex );	
 };

struct CameraApp : public VulkanApp
{
protected:
	CameraPositioner_FirstPerson positioner_;
	Camera camera_;

public:
	CameraApp ( int screenWidth, int screenHeight)
		: VulkanApp ( screenWidth, screenHeight )
		, positioner_ ( vec3(0.0f, 5.0f, 10.0f), vec3(0.0f, 0.0f, - 1.0f), vec3(0.0f, -1.0f, 0.0f))
		, camera_(positioner_)
	{
	}
		
	virtual void update ( float dt ) override
	{
		positioner_.update ( dt, mouseState_.pos, shouldHandleMouse() && mouseState_.leftButtonPressed );
	}
	
	virtual void handleKey ( int key, bool pressed ) override
	{
		if ( key == GLFW_KEY_W ) positioner_.movement_.forward_ = pressed;
		if ( key == GLFW_KEY_S ) positioner_.movement_.backward_ = pressed;
		if ( key == GLFW_KEY_A ) positioner_.movement_.left_ = pressed;
		if ( key == GLFW_KEY_D ) positioner_.movement_.right_ = pressed;
		if ( key == GLFW_KEY_C ) positioner_.movement_.up_ = pressed;
		if ( key == GLFW_KEY_E ) positioner_.movement_.down_ = pressed;
	}
	
	glm::mat4 getDefaultProjection() const
	{
		const float ratio = ctx_.vkDev_.framebufferWidth / (float)ctx_.vkDev_.framebufferHeight;
		return glm::perspective ( glm::pi<float> () / 4.0f, ratio, 0.1f, 1000.0f );
	}
};

#endif // __VULKAN_APP_H__