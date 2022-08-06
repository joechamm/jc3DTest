#ifndef __MULTI_RENDERER_H__
#define __MULTI_RENDERER_H__

#include <jcVkFramework/vkFramework/Renderer.h>
#include <jcCommonFramework/scene/Scene.h>
#include <jcCommonFramework/scene/Material.h>
#include <jcCommonFramework/scene/VtxData.h>

#include <jcCommonFramework/ResourceString.h>

#include <taskflow/taskflow/taskflow.hpp>

// for right now store the default vert and frag shaders as constexpr
constexpr const char* DefaultMeshVertexShader = ROOT_DIR "assets/shaders/VK_DefaultMesh.vert";
constexpr const char* DefaultMeshFragmentShader = ROOT_DIR "assets/shaders/VK_DefaultMesh.frag";

// Container of mesh data, material data and scene nodes with transformations
struct VKSceneData
{
	VulkanTexture envMapIrradiance_;
	VulkanTexture envMap_;
	VulkanTexture brdfLUT_;

	VulkanBuffer material_;
	VulkanBuffer transforms_;

	VulkanRenderContext& ctx_;

	TextureArrayAttachment allMaterialTextures_;
	
	BufferAttachment indexBuffer_;
	BufferAttachment vertexBuffer_;
	
	MeshData meshData_;

	Scene scene_;
	std::vector<MaterialDescription> materials_;
	
	std::vector<glm::mat4> shapeTransforms_;
	std::vector<DrawData> shapes_;

	/* Chapter 9, async loading */
	struct LoadedImageData
	{
		int index_ = 0;
		int w_ = 0;
		int h_ = 0;
		const uint8_t* img_ = nullptr;
	};

	std::vector<std::string> textureFiles_;
	std::vector<LoadedImageData> loadedFiles_;
	std::mutex loadedFilesMutex_;

private:
	tf::Taskflow taskflow_;
	tf::Executor executor_;

public:
	VKSceneData ( VulkanRenderContext& ctx,
		const std::string& meshFile,
		const std::string& sceneFile,
		const std::string& materialFile,
		VulkanTexture envMap,
		VulkanTexture irradianceMap,
		bool asyncLoaded = false);

	void loadScene ( const std::string& sceneFile );
	void loadMeshes ( const std::string& meshFile );

	void convertGlobalToShapeTransforms ();
	void recalculateAllTransforms ();
	void uploadGlobalTransforms ();

	void updateMaterial ( int matIdx );
};

struct MultiRenderer : public Renderer
{
private:
	VKSceneData& sceneData_;
	
	std::vector<VulkanBuffer> indirect_;
	std::vector<VulkanBuffer> shape_;

	struct UBO
	{
		glm::mat4 proj_;
		glm::mat4 view_;
		glm::vec4 cameraPos_;
	} ubo_;

public:
	MultiRenderer ( 
		VulkanRenderContext& ctx,
		VKSceneData& sceneData,
		const char* vtxShaderFile = DefaultMeshVertexShader,
		const char* fragShaderFile = DefaultMeshFragmentShader,
		const std::vector<VulkanTexture>& outputs = std::vector<VulkanTexture>{},
		RenderPass screenRenderPass = RenderPass (),
		const std::vector<BufferAttachment>& auxBuffers = std::vector<BufferAttachment>{},
		const std::vector<TextureAttachment>& auxTextures = std::vector<TextureAttachment>{}
	);

	void fillCommandBuffer(VkCommandBuffer cmdBuffer, size_t currentImage, VkFramebuffer fb = VK_NULL_HANDLE, VkRenderPass rp = VK_NULL_HANDLE) override;
	void updateBuffers ( size_t currentImage ) override;
	
	void updateIndirectBuffers ( size_t currentImage, bool* visibility = nullptr );

	inline void setMatrices ( const glm::mat4& proj, const glm::mat4& view )
	{
		const glm::mat4 m1 = glm::scale ( glm::mat4 ( 1.0f ), glm::vec3 ( 1.0f, -1.0f, 1.0f ) );
		ubo_.proj_ = proj;
		ubo_.view_ = view * m1;
	}

	inline void setCameraPosition ( const glm::vec3& cameraPos )
	{
		ubo_.cameraPos_ = glm::vec4 ( cameraPos, 1.0f );
	}

	inline const VKSceneData& getSceneData () const
	{
		return sceneData_; 
	}

	// Async loading in Chapter 9
	bool checkLoadedTextures ();
};

#endif // !__MULTI_RENDERER_H__
