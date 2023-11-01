#include "Scene.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

using namespace engine;

Scene::Scene() {
	_Renderer = new Renderer();
	CHECK(_Renderer);
}

Scene::~Scene() {
	_Renderer = nullptr;
}

void Scene::InitScene() {
	_Renderer->Init();

	UploadMesh("../asset/model/room.obj", "room");
	UploadMesh("../asset/model/sponza.obj", "sponza");
	UploadMesh("../asset/model/ball.obj", "ball");
	UploadTriangleMesh();

	Material deafaultMaterial;
	_Renderer->CreatePipeline(deafaultMaterial);
	_Materials["Default"] = deafaultMaterial;

	RenderObject monkey;
	monkey.mesh = GetMesh("room");
	monkey.material = GetMaterial("Default");
	monkey.transformMatrix = glm::rotate(glm::mat4{1.0}, -90.0f, glm::vec3{1, 0, 0});

	_Renderables.push_back(monkey);

	for (int x = -20; x <= 20; x++) {
		for (int y = -20; y <= 20; y++) {

			RenderObject tri;
			tri.mesh = GetMesh("Triangle");
			tri.material = GetMaterial("Default");
			glm::mat4 translation = glm::translate(glm::mat4{ 1.0 }, glm::vec3(x, 0, y));
			glm::mat4 scale = glm::scale(glm::mat4{ 1.0 }, glm::vec3(0.2, 0.2, 0.2));
			tri.transformMatrix = translation * scale;

			_Renderables.push_back(tri);
		}
	}
}

void Scene::Update() {
  int count = 1;
	for (int x = -20; x <= 20; x++) {
		for (int y = -20; y <= 20; y++) {

			RenderObject& tri = _Renderables[count++];
			glm::mat4 translation = glm::translate(glm::mat4{1.0f}, glm::vec3(x, 0, y));
			glm::mat4 rotate = glm::rotate_slow(glm::mat4{ 1.0 }, glm::radians(_FrameCount / 0.4f), glm::vec3{ 0, 1, 0 });
			glm::mat4 scale = glm::scale(glm::mat4{ 1.0 }, glm::vec3(0.2, 0.2, 0.2));
			tri.transformMatrix = translation * rotate * scale;

		}
	}

	_Renderer->Draw(_Renderables.data(), (int)_Renderables.size());
	_FrameCount++;
}

void Scene::UpdatePosition(SDL_Event event) {
    // Move 
    switch (event.key.keysym.sym) {
        case SDLK_w :{
            _Camera.keyBoardSpeedZ = 10;
            break;
        }
        case SDLK_s :{
            _Camera.keyBoardSpeedZ = -10;
            break;
        }
        case SDLK_a :{
            _Camera.keyBoardSpeedX = -10;
            break;
        }
        case SDLK_d :{
            _Camera.keyBoardSpeedX = 10;
            break;
        }
        case SDLK_q :{
            _Camera.keyBoardSpeedY = 10;
            break;
        }
        case SDLK_e :{
            _Camera.keyBoardSpeedY = -10;
            break;
        }
    }
       
    _Camera.UpdateCameraPosition();
    ((Renderer*)_Renderer)->UpdateViewMat(_Camera.GetViewMatrix());
}

void Scene::UploadMesh(const char* filename, const char* mesh_name) {
	Mesh tempMesh;
	tempMesh.LoadFromObj(filename);
	((Renderer*)_Renderer)->UploadMeshes(tempMesh);
	_Meshes[mesh_name] = tempMesh;
}

void Scene::UploadTriangleMesh() {

	INFO("Loading triangle");
	Mesh tempMesh;
	//make the array 3 vertices long
	tempMesh.vertices.resize(3);

	//vertex positions
	tempMesh.vertices[0].position = { 1.f,1.f, 0.5f };
	tempMesh.vertices[1].position = { -1.f,1.f, 0.5f };
	tempMesh.vertices[2].position = { 0.f,-1.f, 0.5f };

	//vertex colors, all green
	tempMesh.vertices[0].color = { 1.f, 0.f, 0.0f }; //pure green
	tempMesh.vertices[1].color = { 0.f, 1.f, 0.0f }; //pure green
	tempMesh.vertices[2].color = { 0.f, 0.f, 1.0f }; //pure green
	((Renderer*)_Renderer)->UploadMeshes(tempMesh);
	_Meshes["Triangle"] = tempMesh;
}

//returns nullptr if it can't be found
Material* Scene::GetMaterial(const std::string& name) {
	//search for the object, and return nullptr if not found
	auto it = _Materials.find(name);
	if (it == _Materials.end()) {
		return nullptr;
	}
	else {
		return &(*it).second;
	}
}

//returns nullptr if it can't be found
Mesh* Scene::GetMesh(const std::string& name) {
	auto it = _Meshes.find(name);
	if (it == _Meshes.end()) {
		return nullptr;
	}
	else {
		return &(*it).second;
	}
}

void Scene::Destroy() {
	if (_Renderer != nullptr)
	{
		_Renderer->Release();
		free(_Renderer);
	}
}
