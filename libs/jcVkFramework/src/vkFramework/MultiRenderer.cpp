#include <jcVkFramework/vkFramework/MultiRenderer.h>
#include <jcVkFramework/ResourceString.h>
#include <stb/stb_image.h>

uint8_t* genDefaultCheckerboardImage ( int* width, int* height );

VKSceneData::VKSceneData ( VulkanRenderContext& ctx, const std::string& meshFile, const std::string& sceneFile, const std::string& materialFile, VulkanTexture envMap, VulkanTexture irradianceMap, bool asyncLoad )
	: ctx_(ctx)
	, envMapIrradiance_(irradianceMap)
	, envMap_(envMap)
{
	brdfLUT_ = ctx.resources_.loadKTX ( appendToRoot ( "assets/images/brdfLUT.ktx" ).c_str () );

	loadMaterials ( materialFile.c_str (), materials_, textureFiles_ );

	std::vector<VulkanTexture> textures;
	for ( const auto& f : textureFiles_ )
	{
		auto t = asyncLoad ? ctx.resources_.add
	}
	
}

void VKSceneData::loadScene ( const std::string& sceneFile )
{}

void VKSceneData::loadMeshes ( const std::string & meshFile )
{}

void VKSceneData::convertGlobalToShapeTransforms ()
{}

void VKSceneData::recalculateAllTransforms ()
{}

void VKSceneData::uploadGlobalTransforms ()
{}

void VKSceneData::updateMaterial ( int matIdx )
{}

MultiRenderer::MultiRenderer ( VulkanRenderContext& ctx, VKSceneData& sceneData, const std::string& vtxShaderFile, const std::string& fragShaderFile, const std::vector<VulkanTexture>& outputs, RenderPass screenRenderPass, const std::vector<BufferAttachment>& auxBuffers, const std::vector<TextureAttachment>& auxTextures )
{}

void MultiRenderer::fillCommandBuffer ( VkCommandBuffer cmdBuffer, size_t currentImage, VkFramebuffer fb, VkRenderPass rp )
{}

void MultiRenderer::updateBuffers ( size_t currentImage )
{}

void MultiRenderer::updateIndirectBuffers ( size_t currentImage, bool* visibility )
{}

bool MultiRenderer::checkLoadedTextures ()
{
	return false;
}
