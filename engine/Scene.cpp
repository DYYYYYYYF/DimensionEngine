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
	UploadMeshes();
	Material deafaultMaterial;
	_Renderer->CreatePipeline(deafaultMaterial);
	_Materials["Default"] = deafaultMaterial;

	RenderObject monkey;
	monkey.mesh = GetMesh("monkey");
	monkey.material = GetMaterial("Default");
	monkey.transformMatrix = glm::mat4{ 1.0f };

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
	_Renderer->Draw(_Renderables.data(), _Renderables.size());
}

void Scene::UploadMeshes() {

	INFO("Loading triangle");
	//make the array 3 vertices long
	_TriangleMesh.vertices.resize(3);

	//vertex positions
	_TriangleMesh.vertices[0].position = { 1.f,1.f, 0.5f };
	_TriangleMesh.vertices[1].position = { -1.f,1.f, 0.5f };
	_TriangleMesh.vertices[2].position = { 0.f,-1.f, 0.5f };

	//vertex colors, all green
	_TriangleMesh.vertices[0].color = { 1.f, 0.f, 0.0f }; //pure green
	_TriangleMesh.vertices[1].color = { 0.f, 1.f, 0.0f }; //pure green
	_TriangleMesh.vertices[2].color = { 0.f, 0.f, 1.0f }; //pure green

	//we don't a
	_MonkeyMesh.LoadFromObj("../asset/model/ball.obj");

	// UpLoadMeshes(_TriangleMesh);
	((Renderer*)_Renderer)->UploadMeshes(_MonkeyMesh);
	((Renderer*)_Renderer)->UploadMeshes(_TriangleMesh);

	_Meshes["monkey"] = _MonkeyMesh;
	_Meshes["Triangle"] = _TriangleMesh;
}

//create material and add it to the map
Material* Scene::CreateMaterial(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name) {
	
	Material mat;
	mat.pipeline = pipeline;
	mat.pipelineLayout = layout;
	_Materials[name] = mat;
	return &_Materials[name];
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

//our draw function
void Scene::DrawObjects(vk::CommandBuffer cmd, RenderObject* first, int count) {

}


void Scene::Destroy() {
	if (_Renderer != nullptr)
	{
		_Renderer->Release();
		free(_Renderer);
	}
}
