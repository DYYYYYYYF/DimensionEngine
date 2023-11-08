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
	UploadMesh("../asset/model/sponza.obj", "sponza");
	UploadMesh("../asset/model/ball.obj", "ball");
	UploadMesh("../asset/model/bunny.obj", "Bunny");
	UploadMesh("../asset/model/CornellBox.obj", "CornellBox");
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

	RenderObject monkey;
	monkey.mesh = GetMesh("Rectangle");
	monkey.material = GetMaterial("Default");
	
	monkey.SetScale(1.0f);
	monkey.SetRotate(glm::vec3{ 0, 1, 0 }, 90.0f);

	_Renderables.push_back(monkey);

	for (int x = -20; x <= 20; x++) {
		for (int y = -20; y <= 20; y++) {

			RenderObject tri;
			tri.mesh = GetMesh("Triangle");
			tri.material = GetMaterial("Default");
			tri.SetScale(0.2f);
			tri.SetTranslate(glm::vec3(x, 0, y));

			_Renderables.push_back(tri);
		}
	}
}

void Scene::Update() {
  int count = 1;
	for (int x = -20; x <= 20; x++) {
		for (int y = -20; y <= 20; y++) {

			RenderObject& tri = _Renderables[count++];
			tri.SetRotate(glm::vec3{ 0, 1, 0 }, _FrameCount / 4.f);

		}
	}

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
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
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
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
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

	Mesh tempMesh;
	//make the array 3 vertices long
	tempMesh.vertices.resize(3);

	//vertex positions
	tempMesh.vertices[0].position = { 1.f,2.f, 0.5f };
	tempMesh.vertices[1].position = { -1.f,2.f, 0.5f };
	tempMesh.vertices[2].position = { 0.f,0.f, 0.5f };

	//vertex colors, all green
	tempMesh.vertices[0].color = { 1.f, 0.f, 0.0f }; //pure green
	tempMesh.vertices[1].color = { 0.f, 1.f, 0.0f }; //pure green
	tempMesh.vertices[2].color = { 0.f, 0.f, 1.0f }; //pure green
	((Renderer*)_Renderer)->UploadMeshes(tempMesh);
	_Meshes["Triangle"] = tempMesh;

	INFO("Loaded Triangle");
}

void Scene::UploadRectangleMesh() {
	Mesh tempMesh;
	//make the array 6 vertices long
	tempMesh.vertices.resize(6);

	//vertex positions
	tempMesh.vertices[0].position = {  20.f, 0.f, -20.f };
	tempMesh.vertices[1].position = {  20.f, 0.f,  20.f };
	tempMesh.vertices[2].position = { -20.f, 0.f,  20.f };

	tempMesh.vertices[3].position = { -20.f, 0.f,   20.f };
	tempMesh.vertices[4].position = { -20.f, 0.f,  -20.f };
	tempMesh.vertices[5].position = {  20.f, 0.0f, -20.f };

	//vertex colors, all green
	tempMesh.vertices[0].color = { 0.7f, 0.7f, 0.7f }; 
	tempMesh.vertices[1].color = { 0.3f, 0.3f, 0.3f };
	tempMesh.vertices[2].color = { 0.7f, 0.7f, 0.7f };
	tempMesh.vertices[3].color = { 0.7f, 0.7f, 0.7f };
	tempMesh.vertices[4].color = { 0.3f, 0.3f, 0.3f };
	tempMesh.vertices[5].color = { 0.7f, 0.7f, 0.7f };
	((Renderer*)_Renderer)->UploadMeshes(tempMesh);
	_Meshes["Rectangle"] = tempMesh;
	
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
