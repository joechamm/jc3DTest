#ifndef __COMPOSITE_RENDERER_H__
#define __COMPOSITE_RENDERER_H__

#include <jcVkFramework/vkFramework/VulkanApp.h>
#include <jcVkFramework/vkFramework/Renderer.h>

struct CompositeRenderer : public Renderer
{
protected:
	std::vector<RenderItem> renderers_;

public:
	CompositeRenderer ( VulkanRenderContext& c ) : Renderer ( c ) {}
	
	void fillCommandBuffer ( VkCommandBuffer cmdBuffer, size_t currentImage, VkFramebuffer fb1 = VK_NULL_HANDLE, VkRenderPass rp1 = VK_NULL_HANDLE ) override
	{
		for ( auto& r : renderers_ )
		{
			if ( r.enabled_ )
			{
				VkRenderPass rp = rp1;
				VkFramebuffer fb = fb1;

				if ( r.renderer_.renderPass_.handle_ != VK_NULL_HANDLE )
				{
					rp = r.renderer_.renderPass_.handle_;
				}
				
				if ( r.renderer_.framebuffer_ != VK_NULL_HANDLE )
				{
					fb = r.renderer_.framebuffer_;
				}

				r.renderer_.fillCommandBuffer ( cmdBuffer, currentImage, fb, rp );
			}
		}
	}

	void updateBuffers ( size_t currentImage ) override
	{
		for ( auto& r : renderers_ )
		{
			r.renderer_.updateBuffers ( currentImage );
		}
	}
	
};

#endif // !__COMPOSITE_RENDERER_H__
