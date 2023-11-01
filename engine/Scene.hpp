#pragma once

#include <unordered_map>
#include <SDL2/SDL.h>
#include "EngineStructures.hpp"
#include "../renderer/Renderer.hpp"
#include "foundation/Camera.hpp"

using namespace renderer;

namespace engine {
class Scene {
public:
	Scene();
	virtual ~Scene();

	void InitScene();
	void Update();
	void Destroy();

	void UploadMesh(const char* filename, const char* mesh_name);
	void UploadTriangleMesh();
	Material* GetMaterial(const std::string& name);
	Mesh* GetMesh(const std::string& name);
	void UpdatePosition();

private:
	IRenderer* _Renderer;

	//default array of renderable objects
	std::vector<RenderObject> _Renderables;
	std::unordered_map<std::string, Material> _Materials;
	std::unordered_map<std::string, Mesh> _Meshes;
	Camera* _Camera;

public:
	uint32_t _FrameCount = 1;

};
}
