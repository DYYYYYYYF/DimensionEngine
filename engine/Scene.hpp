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
	void UpdatePosition(SDL_Event event);

	Material* GetMaterial(const std::string& name);
	Mesh* GetMesh(const std::string& name);

private:
	IRenderer* _Renderer;

	//default array of renderable objects
	std::vector<RenderObject> _Renderables;
	std::unordered_map<std::string, Material> _Materials;
	std::unordered_map<std::string, Mesh> _Meshes;

	Camera _Camera = Camera(glm::vec3(0.0f, 10.0f, -10.0f), glm::radians(-45.0f), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

public:
	uint32_t _FrameCount = 1;

};
}
