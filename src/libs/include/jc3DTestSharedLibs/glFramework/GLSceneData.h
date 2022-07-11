#pragma once

#include <jc3DTestSharedLibs/scene/Scene.h>
#include <jc3DTestSharedLibs/scene/Material.h>
#include <jc3DTestSharedLibs/scene/VtxData.h>
#include <jc3DTestSharedLibs/glFramework/GLShader.h>
#include <jc3DTestSharedLibs/glFramework/GLTexture.h>

class GLSceneData
{
public:
	GLSceneData(
		const char* meshFile,
		const char* sceneFile,
		const char* materialFile);

	std::vector<GLTexture> allMaterialTextures_;

	MeshFileHeader header_;
	MeshData meshData_;

	Scene scene_;
	std::vector<MaterialDescription> materials_;
	std::vector<DrawData> shapes_;

	void loadScene(const char* sceneFile);
};
