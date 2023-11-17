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
	UploadMesh("../asset/model/ball.obj", "ball");
	UploadMesh("../asset/model/CornellBox.obj", "CornellBox");

	UploadMesh("../asset/model/sponza.obj", "sponza");
	UploadMesh("../asset/model/bunny.obj", "Bunny");
	UploadMesh("../asset/obj/wooden_boat/Boat.obj", "Boat");
	
	UploadTriangleMesh();
	UploadRectangleMesh();

	const char* defaultVertShader = "../shader/glsl/default_vert.spv";
	const char* defaultFragShader = "../shader/glsl/default_frag.spv";
	const char* meshFragShader = "../shader/glsl/texture_mesh_frag.spv";
	const char* meshFloorVertShader = "../shader/glsl/mesh_floor_vert.spv";
	const char* meshFloorFragShader = "../shader/glsl/mesh_floor_frag.spv";

	Material deafaultMaterial;
	_Renderer->CreatePipeline(deafaultMaterial, defaultVertShader, defaultFragShader);
	_Materials["Default"] = deafaultMaterial;

	Material deafaultMeshMaterial;
	_Renderer->CreatePipeline(deafaultMeshMaterial, defaultVertShader, meshFragShader);
	_Materials["Texture"] = deafaultMeshMaterial;

	Material deafaultFloorMaterial;
	_Renderer->CreatePipeline(deafaultFloorMaterial, meshFloorVertShader, meshFloorFragShader);
	_Materials["Floor"] = deafaultFloorMaterial;

	Material drawLineMaterial;
	((Renderer*)_Renderer)->CreateDrawlinePipeline(drawLineMaterial);
	_Materials["DrawLine"] = drawLineMaterial;

	RenderObject floor;
	floor.mesh = GetMesh("Rectangle");
	floor.material = GetMaterial("Floor");
	_Renderables.push_back(floor);

	RenderObject triangle;
	triangle.mesh = GetMesh("room");
	triangle.material = GetMaterial("Texture");
	triangle.SetTranslate(glm::vec3{ 0, 1, 0 });
	triangle.SetRotate(glm::vec3{ 0, 0, 1 }, 90);
	triangle.SetRotate(glm::vec3{ 0, 1, 0 }, 90);
	triangle.SetRotate(glm::vec3{ 1, 0, 0 }, -90);
	_Renderables.push_back(triangle);

	INFO("Inited Scene.");
}

void Scene::Update() {

	_Renderer->BeforeDraw();
	_Renderer->Draw(_Renderables.data(), (int)_Renderables.size());
	_Renderer->AfterDraw();

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

			_Camera.MouserMovement((float)-x, (float)-y);
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
	triganleMesh.vertices[0].position = { 1.f, 2.f, 0.0f };
	triganleMesh.vertices[1].position = { -1.f,2.f, 0.0f };
	triganleMesh.vertices[2].position = { 0.f, 1.f, 0.0f };

	//vertex colors
	triganleMesh.vertices[0].color = { 1.f, 0.f, 0.0f };
	triganleMesh.vertices[1].color = { 0.f, 1.f, 0.0f };
	triganleMesh.vertices[2].color = { 0.f, 0.f, 1.0f };

	// Normal
	triganleMesh.vertices[0].normal = { 0.0f, 1.0f, 0.0f };
	triganleMesh.vertices[1].normal = { 0.0f, 1.0f, 0.0f };
	triganleMesh.vertices[2].normal = { 0.0f, 1.0f, 0.0f };

	// UV
	triganleMesh.vertices[0].texCoord = { 0.0f, 0.0f };
	triganleMesh.vertices[1].texCoord = { 1.0f, 0.0f };
	triganleMesh.vertices[2].texCoord = { 0.5f, 1.0f };

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
	rectangleMesh.vertices[0].position = {  50.f, 0.f,  -50.f };	//右后
	rectangleMesh.vertices[1].position = {  50.f, 0.f,   50.f };	//右前
	rectangleMesh.vertices[2].position = { -50.f, 0.f,   50.f };	//左前
	rectangleMesh.vertices[3].position = { -50.f, 0.f,  -50.f };	//左后

	//vertex colors, all green
	rectangleMesh.vertices[0].color = { 0.9f, 0.9f, 0.9f };
	rectangleMesh.vertices[1].color = { 0.1f, 0.1f, 0.1f };
	rectangleMesh.vertices[2].color = { 0.9f, 0.9f, 0.9f };
	rectangleMesh.vertices[3].color = { 0.1f, 0.1f, 0.1f };

	// Normal
	rectangleMesh.vertices[0].normal = { 0.0f, 1.0f, 0.0f };
	rectangleMesh.vertices[1].normal = { 0.0f, 1.0f, 0.0f };
	rectangleMesh.vertices[2].normal = { 0.0f, 1.0f, 0.0f };
	rectangleMesh.vertices[3].normal = { 0.0f, 1.0f, 0.0f };

	// UV
	rectangleMesh.vertices[0].texCoord = { 0.0f, 1.0f };
	rectangleMesh.vertices[1].texCoord = { 1.0f, 1.0f };
	rectangleMesh.vertices[2].texCoord = { 1.0f, 0.0f };
	rectangleMesh.vertices[3].texCoord = { 0.0f, 0.0f };

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
    ((Renderer*)_Renderer)->DestroyMaterials(_Materials);
		_Renderer->Release();
		free(_Renderer);
	}
}

RenderObject Scene::GenerateBoudingBox(const Mesh* mesh) {
	
	RenderObject obj;
	obj.material = GetMaterial("DrawLine");

	float fx = 0.0f;
	float fy = 0.0f;
	float fz = 0.0f;

	float nx = 0.0f;
	float ny = 0.0f;
	float nz = 0.0f;

	for (const Vertex& vert : mesh->vertices) {
		if (fx < vert.position[0]) fx = vert.position[0];
		if (fy < vert.position[1]) fy = vert.position[1];
		if (fz < vert.position[2]) fz = vert.position[2];

		if (nx > vert.position[0]) nx = vert.position[0];
		if (ny > vert.position[1]) ny = vert.position[1];
		if (nz > vert.position[2]) nz = vert.position[2];
	}

	Mesh boundingMesh;
	boundingMesh.vertices.resize(8);

	// up mesh
	boundingMesh.vertices[0].position = { fx, fy, fz };
	boundingMesh.vertices[1].position = { fx, fy, nz };
	boundingMesh.vertices[2].position = { nx, fy, fz };
	boundingMesh.vertices[3].position = { nx, fy, nz };
	boundingMesh.vertices[4].position = { fx, ny, fz };
	boundingMesh.vertices[5].position = { nx, ny, fz };
	boundingMesh.vertices[6].position = { fx, ny, nz };
	boundingMesh.vertices[7].position = { nx, ny, nz };

	boundingMesh.vertices[0].color = { 1.0f, 1.0f, 1.0f };
	boundingMesh.vertices[1].color = { 1.0f, 1.0f, 1.0f };
	boundingMesh.vertices[2].color = { 1.0f, 1.0f, 1.0f };
	boundingMesh.vertices[3].color = { 1.0f, 1.0f, 1.0f };
	boundingMesh.vertices[4].color = { 1.0f, 1.0f, 1.0f };
	boundingMesh.vertices[5].color = { 1.0f, 1.0f, 1.0f };
	boundingMesh.vertices[6].color = { 1.0f, 1.0f, 1.0f };
	boundingMesh.vertices[7].color = { 1.0f, 1.0f, 1.0f };

	boundingMesh.indices.resize(16);
	boundingMesh.indices[0] = 0;
	boundingMesh.indices[1] = 1;
	boundingMesh.indices[2] = 3;
	boundingMesh.indices[3] = 2;
	boundingMesh.indices[4] = 0;

	boundingMesh.indices[5] = 4;
	boundingMesh.indices[6] = 6;
	boundingMesh.indices[7] = 7;
	boundingMesh.indices[8] = 5;
	boundingMesh.indices[9] = 2;

	boundingMesh.indices[10] = 3;
	boundingMesh.indices[11] = 7;
	boundingMesh.indices[12] = 5;
	boundingMesh.indices[13] = 4;
	boundingMesh.indices[14] = 6;
	boundingMesh.indices[15] = 1;

	((Renderer*)_Renderer)->UploadMeshes(boundingMesh);
	_Meshes["Bouding"] = boundingMesh;
	obj.mesh = GetMesh("Bouding");
	obj.SetTranslate(glm::vec3{ 0.0f, 1.0f, 0.0f });

	_Renderables.push_back(obj);

	return obj;
}