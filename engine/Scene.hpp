#pragma once

#include <unordered_map>
#include "EngineStructures.hpp"
#include "foundation/Camera.hpp"
#include "../renderer/Renderer.hpp"

#ifdef _WIN32
const int moveSpeed = 1;
#elif __APPLE__
const int moveSpeed = 10;
#endif

using namespace renderer;

namespace engine {
class Scene {
public:
	Scene();
	virtual ~Scene();

	void InitScene();
	void Update();
	void Destroy();
	void UpdatePosition(GLFWwindow* window);

	RenderObject GenerateBoudingBox(const Mesh* mesh);

private:
	IRenderer* _Renderer;
	Camera _Camera = Camera(Vector3(0.0f, 10.0f, -10.0f), glm::radians(0.0f), glm::radians(0.0f), Vector3(0.0f, 1.0f, 0.0f));

	//default array of renderable objects
	std::vector<RenderObject> _Renderables;

public:
	uint32_t _FrameCount = 1;

};
}
