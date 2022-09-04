#ifndef __GLSCENE_DATA_H__
#define __GLSCENE_DATA_H__

#include <jcCommonFramework/scene/Scene.h>
#include <jcCommonFramework/scene/Material.h>
#include <jcCommonFramework/scene/VtxData.h>
#include <jcGLframework/GLShader.h>
#include <jcGLframework/GLTexture.h>
#include <jcGLframework/jcGLframework.h>

namespace jcGLframework
{
	class GLSceneData
	{
	public:
		GLSceneData (
			const char* meshFile,
			const char* sceneFile,
			const char* materialFile );

		std::vector<GLTexture> allMaterialTextures_;

		MeshFileHeader header_;
		MeshData meshData_;

		Scene scene_;
		std::vector<MaterialDescription> materials_;
		std::vector<DrawData> shapes_;

		void loadScene ( const char* sceneFile );
	};
}

#endif // !__GLSCENE_DATA_H__
