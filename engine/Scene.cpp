#include "Scene.hpp"

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
	/*
	UploadMesh("../asset/model/sponza.obj", "sponza");
	UploadMesh("../asset/model/ball.obj", "ball");
	UploadMesh("../asset/model/bunny.obj", "Bunny");
	UploadMesh("../asset/model/CornellBox.obj", "CornellBox");
	UploadMesh("../asset/obj/wooden_boat/Boat.obj", "Boat");
	*/
	UploadTriangleMesh();
	UploadRectangleMesh();

	const char* defaultVertShader = "../shader/default_vert.spv";
	const char* defaultFragShader = "../shader/default_frag.spv";
	const char* meshFragShader = "../shader/texture_mesh_frag.spv";;
	const char* meshFloorFragShader = "../shader/mesh_floor_frag.spv";;

	Material deafaultMaterial;
	_Renderer->CreatePipeline(deafaultMaterial, defaultVertShader, defaultFragShader);
	_Materials["Default"] = deafaultMaterial;

	Material deafaultMeshMaterial;
	_Renderer->CreatePipeline(deafaultMeshMaterial, defaultVertShader, meshFragShader);
	_Materials["Texture"] = deafaultMeshMaterial;

	Material deafaultFloorMaterial;
	_Renderer->CreatePipeline(deafaultFloorMaterial, defaultVertShader, meshFloorFragShader);
	_Materials["Floor"] = deafaultFloorMaterial;

	RenderObject floor;
	floor.mesh = GetMesh("Rectangle");
	floor.material = GetMaterial("Default");
	floor.SetScale(1.0f);
	floor.SetRotate(glm::vec3{ 0, 1, 0 }, 90.0f);
	_Renderables.push_back(floor);

	RenderObject boat;
	boat.mesh = GetMesh("Triangle");
	boat.material = GetMaterial("Texture");
	boat.SetTranslate(glm::vec3{0.0f, 1.0f, 0.0f});
	boat.SetScale(glm::vec3{2.f, 4.f, 2.f});
	_Renderables.push_back(boat);

	INFO("Inited Scene.");
}

void Scene::Update() {

	_Renderer->Draw(_Renderables.data(), (int)_Renderables.size());
    _Renderer->WaitIdel();
	_FrameCount++;

}

void Scene::UpdatePosition(GLFWwindow* window) {

	if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
		glfwSetWindowShouldClose(window, true);
	}
	
	if (glfwGetKey(window, GLFW_KEY_W)) {
		_Camera.keyBoardSpeedZ = moveSpeed;      //press-W
	}
	else if (glfwGetKey(window, GLFW_KEY_S)) {
		_Camera.keyBoardSpeedZ = -moveSpeed;     //press-S
	}
	else {
		_Camera.keyBoardSpeedZ = 0;
	}
	if (glfwGetKey(window, GLFW_KEY_A)) {
		_Camera.keyBoardSpeedX = -moveSpeed;     //press-A
	}
	else if (glfwGetKey(window, GLFW_KEY_D)) {
		_Camera.keyBoardSpeedX = moveSpeed;      //press-D
	}
	else {
		_Camera.keyBoardSpeedX = 0;
	}
	if (glfwGetKey(window, GLFW_KEY_Q)) {
		_Camera.keyBoardSpeedY = moveSpeed;      //press-Q
	}
	else if (glfwGetKey(window, GLFW_KEY_E)) {
		_Camera.keyBoardSpeedY = -moveSpeed;     //press-E
	}
	else {
		_Camera.keyBoardSpeedY = 0;
	}

	// Mouse
	double posX = 0, posY;
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
		//press mouse-left
		glfwGetCursorPos(window, &posX, &posY);
		if (_Camera.mouesePos.x == 0 && _Camera.mouesePos.y == 0) {
			_Camera.mouesePos.x = posX;
			_Camera.mouesePos.y = posY;
		}
		else {
			double x = posX - _Camera.mouesePos.x;
			double y = posY - _Camera.mouesePos.y;
			_Camera.mouesePos.x = posX;
			_Camera.mouesePos.y = posY;

			_Camera.MouserMovement(-x, -y);
			// keyBoardScoll.rotateX -= glm::dot(glm::vec3(0, 0, x), glm::vec3(0, 0, y)) * 0.01;
		}
	}
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE) {
		//release mouse-left
		_Camera.mouesePos.x = 0;
		_Camera.mouesePos.y = 0;
	}

	// keyBoardScoll.scale = scale_callback;
       
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

	Mesh triganleMesh;
	//make the array 3 vertices long
	triganleMesh.vertices.resize(3);

	//vertex positions
	triganleMesh.vertices[0].position = { 1.f,2.f, 0.5f };
	triganleMesh.vertices[1].position = { -1.f,2.f, 0.5f };
	triganleMesh.vertices[2].position = { 0.f,1.f, 0.5f };

	//vertex colors, all green
	triganleMesh.vertices[0].color = { 1.f, 0.f, 0.0f }; //pure green
	triganleMesh.vertices[1].color = { 0.f, 1.f, 0.0f }; //pure green
	triganleMesh.vertices[2].color = { 0.f, 0.f, 1.0f }; //pure green

	//indices
	triganleMesh.indices.resize(3);
	triganleMesh.indices[0] = 0;
	triganleMesh.indices[1] = 1;
	triganleMesh.indices[2] = 2;

	((Renderer*)_Renderer)->UploadMeshes(triganleMesh);
	_Meshes["Triangle"] = triganleMesh;

	INFO("Loaded Triangle");
}

void Scene::UploadRectangleMesh() {
	Mesh rectangleMesh;
	//make the array 6 vertices long
	rectangleMesh.vertices.resize(4);

	//vertex positions
	rectangleMesh.vertices[0].position = {  20.f, 0.f, -20.f };
	rectangleMesh.vertices[1].position = {  20.f, 0.f,  20.f };
	rectangleMesh.vertices[2].position = { -20.f, 0.f,  20.f };
	rectangleMesh.vertices[3].position = { -20.f, 0.f,  -20.f };

	//vertex colors, all green
	rectangleMesh.vertices[0].color = { 0.7f, 0.7f, 0.7f };
	rectangleMesh.vertices[1].color = { 0.3f, 0.3f, 0.3f };
	rectangleMesh.vertices[2].color = { 0.7f, 0.7f, 0.7f };
	rectangleMesh.vertices[3].color = { 0.7f, 0.7f, 0.7f };

	//indices
	rectangleMesh.indices.resize(6);
	rectangleMesh.indices[0] = 0;
	rectangleMesh.indices[1] = 1;
	rectangleMesh.indices[2] = 2;
	rectangleMesh.indices[3] = 0;
	rectangleMesh.indices[4] = 2;
	rectangleMesh.indices[5] = 3;

	((Renderer*)_Renderer)->UploadMeshes(rectangleMesh);
	_Meshes["Rectangle"] = rectangleMesh;
	
	INFO("Loaded Rectangle");
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
    ((Renderer*)_Renderer)->UnloadMeshes(_Meshes);
		_Renderer->Release();
		free(_Renderer);
	}
}
