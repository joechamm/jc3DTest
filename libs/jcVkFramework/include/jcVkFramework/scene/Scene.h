#ifndef __SCENE_H__
#define __SCENE_H__

#include <vector>
#include <unordered_map>
#include <string>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

using std::vector;
using std::unordered_map;

using glm::mat4;

struct SceneNode
{
	int mesh_;
	int material_;
	int parent_;
	int firstChild_;
	int rightSibling_;
};

struct Hierarchy
{
	int parent_;
	int firstChild_;
	int nextSibling_;
	int level_;
};

struct Scene
{
	vector<mat4> localTransforms_;
	vector<mat4> globalTransforms_;
	vector<Hierarchy> hierarchy_;
	
	// Meshes for nodes (Node -> Mesh)
	unordererd_map<uint32_t, uint32_t> meshes_;
	// Materials for nodes (Node -> Material) 
	unordererd_map<uint32_t, uint32_t> materialForNode_;
};


#endif // !__SCENE_H__
