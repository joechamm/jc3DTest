#ifndef __GLSCENE_DATA_LAZY_H__
#define __GLSCENE_DATA_LAZY_H__

#include <mutex>

#include <jcGLframework/GLShader.h>
#include <jcGLframework/GLTexture.h>
#include <jcGLframework/jcGLframework.h>
#include <jcCommonFramework/scene/Scene.h>
#include <jcCommonFramework/scene/Material.h>
#include <jcCommonFramework/scene/VtxData.h>
#include <jcCommonFramework/ResourceString.h>

#include <taskflow/taskflow/taskflow.hpp>
namespace jcGLframework
{
	class GLSceneDataLazy
	{
	public:
		struct LoadedImageData
		{
			int index_ = 0;
			int w_ = 0;
			int h_ = 0;
			const uint8_t* img_ = nullptr;
		};

		const std::shared_ptr<GLTexture> dummyTexture_ = std::make_shared<GLTexture> ( GL_TEXTURE_2D, appendToRoot ( "assets/images/const1.bmp" ).c_str () );

		std::vector<std::string> textureFiles_;
		std::vector<LoadedImageData> loadedFiles_;
		std::mutex loadedFilesMutex_;
		std::vector<std::shared_ptr<GLTexture>> allMaterialTextures_;

		MeshFileHeader header_;
		MeshData meshData_;

		Scene scene_;
		std::vector<MaterialDescription> materialsLoaded_; // materials loaded from scene
		std::vector<MaterialDescription> materials_; // materials uploaded to GPU buffers
		std::vector<DrawData> shapes_;

		tf::Taskflow taskflow_;
		tf::Executor executor_;

	public:
		GLSceneDataLazy ( const char* meshFile,
			const char* sceneFile,
			const char* materialFile );

		bool uploadLoadedTextures ();

	private:
		void loadScene ( const char* sceneFile );
		void updateMaterials ();
	};
}

#endif // !__GLSCENE_DATA_LAZY_H__
