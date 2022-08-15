#include <jcVkFramework/vkFramework/VulkanApp.h>
#include <jcVkFramework/vkFramework/LineCanvas.h>
#include <jcVkFramework/vkFramework/QuadRenderer.h>
#include <jcVkFramework/vkFramework/GuiRenderer.h>
#include <jcVkFramework/vkFramework/VulkanShaderProcessor.h>

#include <jcCommonFramework/ResourceString.h>

struct node
{
	uint32_t idx;
	float xx, yy;
};

static_assert( sizeof ( node ) == 3 * sizeof ( uint32_t ) );

float g_Percentage = 0.5f;

struct AtomicRenderer : public Renderer
{
private:
	std::vector<VulkanBuffer> atomics_;
	std::vector<VulkanBuffer> output_;

public:
	AtomicRenderer ( VulkanRenderContext& ctx, VulkanBuffer sizeBuffer ) : Renderer ( ctx )
	{
		const PipelineInfo pInfo = initRenderPass ( PipelineInfo {}, { ctx.resources_.addColorTexture () }, RenderPass (), ctx.screenRenderPass_NoDepth_ );

		uint32_t W = ctx.vkDev_.framebufferWidth;
		uint32_t H = ctx.vkDev_.framebufferHeight;

		const size_t imgCount = ctx.vkDev_.swapchainImages.size ();
		descriptorSets_.resize ( imgCount );
		atomics_.resize ( imgCount );
		output_.resize ( imgCount );

		DescriptorSetInfo dsInfo = {
			.buffers_ = {
				storageBufferAttachment ( VulkanBuffer {}, 0, sizeof ( uint32_t ), VK_SHADER_STAGE_FRAGMENT_BIT ),
				storageBufferAttachment ( VulkanBuffer{}, 0, W * H * sizeof ( node ), VK_SHADER_STAGE_FRAGMENT_BIT ),
				uniformBufferAttachment ( sizeBuffer, 0, 8, VK_SHADER_STAGE_FRAGMENT_BIT )
			}
		};

		descriptorSetLayout_ = ctx.resources_.addDescriptorSetLayout ( dsInfo );
		descriptorPool_ = ctx.resources_.addDescriptorPool ( dsInfo, ( uint32_t ) imgCount );

		for ( size_t i = 0; i < imgCount; i++ )
		{
			atomics_ [ i ] = ctx.resources_.addStorageBuffer ( sizeof ( uint32_t ) );
			output_ [ i ] = ctx.resources_.addStorageBuffer ( W * H * sizeof ( node ) );
			dsInfo.buffers_ [ 0 ].buffer_ = atomics_ [ i ];
			dsInfo.buffers_ [ 1 ].buffer_ = output_ [ i ];

			descriptorSets_ [ i ] = ctx.resources_.addDescriptorSet ( descriptorPool_, descriptorSetLayout_ );
			ctx.resources_.updateDescriptorSet ( descriptorSets_ [ i ], dsInfo );
		}

		initPipeline ( getShaderFilenamesWithRoot ( "assets/shaders/VK10_AtomicTest.vert", "assets/shaders/VK10_AtomicTest.frag" ), pInfo );
	}

	void fillCommandBuffer ( VkCommandBuffer cmdBuffer, size_t currentImage, VkFramebuffer fb = VK_NULL_HANDLE, VkRenderPass rp = VK_NULL_HANDLE ) override
	{
		beginRenderPass ( ( rp != VK_NULL_HANDLE ) ? rp : renderPass_.handle_, ( fb != VK_NULL_HANDLE ) ? fb : framebuffer_, cmdBuffer, currentImage );
		vkCmdDraw ( cmdBuffer, 6, 1, 0, 0 );
		vkCmdEndRenderPass ( cmdBuffer );
	}

	void updateBuffers ( size_t currentImage ) override
	{
		uint32_t zeroCount = 0;
		uploadBufferData ( ctx_.vkDev_, atomics_ [ currentImage ].memory, 0, &zeroCount, sizeof ( uint32_t ) );
	}

	std::vector<VulkanBuffer>& getOutputs () { return output_; }
};

struct AnimRenderer : public Renderer
{
private:
	std::vector<VulkanBuffer>& pointBuffers_;

public:
	AnimRenderer ( VulkanRenderContext& ctx, std::vector<VulkanBuffer>& pointBuffers, VulkanBuffer sizeBuffer ) : Renderer ( ctx ), pointBuffers_ ( pointBuffers )
	{
		initRenderPass ( PipelineInfo {}, {}, RenderPass (), ctx.screenRenderPass_NoDepth_ );

		uint32_t W = ctx.vkDev_.framebufferWidth;
		uint32_t H = ctx.vkDev_.framebufferHeight;

		const size_t imgCount = ctx.vkDev_.swapchainImages.size ();
		descriptorSets_.resize ( imgCount );

		DescriptorSetInfo dsInfo = {
			.buffers_ = {
				storageBufferAttachment ( VulkanBuffer{}, 0, W * H * sizeof ( node ), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT ),
				uniformBufferAttachment ( sizeBuffer, 0, 8, VK_SHADER_STAGE_VERTEX_BIT )
				}
		};

		descriptorSetLayout_ = ctx.resources_.addDescriptorSetLayout ( dsInfo );
		descriptorPool_ = ctx.resources_.addDescriptorPool ( dsInfo, ( uint32_t ) imgCount );

		for ( size_t i = 0; i < imgCount; i++ )
		{
			dsInfo.buffers_ [ 0 ].buffer_ = pointBuffers_ [ i ];
			descriptorSets_ [ i ] = ctx.resources_.addDescriptorSet ( descriptorPool_, descriptorSetLayout_ );
			ctx.resources_.updateDescriptorSet ( descriptorSets_ [ i ], dsInfo );
		}

		initPipeline ( getShaderFilenamesWithRoot ( "assets/shaders/VK10_AtomicVisualize.vert", "assets/shaders/VK10_AtomicVisualize.frag" ), PipelineInfo { .topology_ = VK_PRIMITIVE_TOPOLOGY_POINT_LIST } );
	}

	void fillCommandBuffer ( VkCommandBuffer cmdBuffer, size_t currentImage, VkFramebuffer fb = VK_NULL_HANDLE, VkRenderPass rp = VK_NULL_HANDLE ) override
	{
		uint32_t pointCount = uint32_t ( ctx_.vkDev_.framebufferWidth * ctx_.vkDev_.framebufferHeight * g_Percentage );
		if ( pointCount == 0 ) return;

		beginRenderPass ( ( rp != VK_NULL_HANDLE ) ? rp : renderPass_.handle_, ( fb != VK_NULL_HANDLE ) ? fb : framebuffer_, cmdBuffer, currentImage );
		vkCmdDraw ( cmdBuffer, pointCount, 1, 0, 0 );
		vkCmdEndRenderPass ( cmdBuffer );
	}
};

struct MyApp : public CameraApp
{
private:
	VulkanBuffer sizeBuffer_;

	AtomicRenderer atom_;
	AnimRenderer anim_;
	GuiRenderer imgui_;

public:
	MyApp () :
		CameraApp ( 1600, 900, VulkanContextFeatures { .vertexPipelineStoresAndAtomics_ = true, .fragmentStoresAndAtomics_ = true } )
		, sizeBuffer_ ( ctx_.resources_.addUniformBuffer ( 8 ) )
		, atom_ ( ctx_, sizeBuffer_ )
		, anim_ ( ctx_, atom_.getOutputs (), sizeBuffer_ )
		, imgui_ ( ctx_, std::vector<VulkanTexture>{} )
	{
		onScreenRenderers_.emplace_back ( atom_, false );
		onScreenRenderers_.emplace_back ( anim_, false );
		onScreenRenderers_.emplace_back ( imgui_, false );
		
		struct WH
		{
			float w, h;
		} wh {
			( float ) ctx_.vkDev_.framebufferWidth,
			( float ) ctx_.vkDev_.framebufferHeight
		};
		
		uploadBufferData ( ctx_.vkDev_, sizeBuffer_.memory, 0, &wh, sizeof ( wh ) );
	}

	void draw3D () override {}

	void drawUI () override
	{
		ImGui::Begin ( "Settings", nullptr );
		ImGui::SliderFloat ( "Percentage", &g_Percentage, 0.0f, 1.0f );
		ImGui::End ();
	}
};

int main ()
{
	MyApp app;
	app.mainLoop ();
	return 0;
}
