#pragma once

#include <unordered_map>
#include "EngineStructures.hpp"
#include "../renderer/Renderer.hpp"

using namespace renderer;

namespace engine {
class Scene {
public:
	Scene();
	virtual ~Scene();

	void InitScene();
	void Update();
	void Destroy();

	void UploadMeshes();
	Material* CreateMaterial(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name);
	Material* GetMaterial(const std::string& name);
	Mesh* GetMesh(const std::string& name);
	void DrawObjects(vk::CommandBuffer cmd, RenderObject* first, int count);

private:
	IRenderer* _Renderer;
	//default array of renderable objects
	std::vector<RenderObject> _Renderables;
	std::unordered_map<std::string, Material> _Materials;
	std::unordered_map<std::string, Mesh> _Meshes;

	Mesh _TriangleMesh;
	Mesh _MonkeyMesh;

};
}