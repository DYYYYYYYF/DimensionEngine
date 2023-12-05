#include "Scene.hpp"

using namespace engine;

Scene::Scene() {
	_Renderer = new Renderer();
	ASSERT(_Renderer);
}

Scene::~Scene() {
	_Renderer = nullptr;
}

void Scene::InitScene(ConfigFile* config) {

	if (config != nullptr) {
		((Renderer*)_Renderer)->SetConfigFile(config);
	}
	_Renderer->Init();
	
	RenderObject floor;
	Mesh* rectMesh = ((Renderer*)_Renderer)->GetMesh("Rectangle");
	Material* floorMat = ((Renderer*)_Renderer)->GetMaterial("Default");
	if (rectMesh != nullptr && floorMat != nullptr) {
		floor.mesh = rectMesh;
		floor.material = floorMat;
		floor.SetTranslate(glm::vec3{ 1, 0, 0 });
		_Renderables.push_back(floor);
	}

	RenderObject room;
	Mesh* roomMesh = ((Renderer*)_Renderer)->GetMesh("room");
	Material* textureMat = ((Renderer*)_Renderer)->GetMaterial("Texture");
	if (roomMesh != nullptr && textureMat != nullptr) {
		room.mesh = roomMesh;
		room.material = textureMat;
		room.SetTranslate(glm::vec3{ 0, 1, 0 });
		room.SetRotate(glm::vec3{ 0, 0, 1 }, 90);
		room.SetRotate(glm::vec3{ 0, 1, 0 }, 90);
		room.SetRotate(glm::vec3{ 1, 0, 0 }, -90);
		_Renderables.push_back(room);
	}

	RenderObject boat;
	Mesh* boatMesh = ((Renderer*)_Renderer)->GetMesh("Boat");
	Material* boatMat = ((Renderer*)_Renderer)->GetMaterial("Default");
	if (boatMesh != nullptr && boatMat != nullptr) {
		boat.mesh = boatMesh;
		boat.material = boatMat;
    boat.SetTranslate(glm::vec3{0, 0, 10});
		_Renderables.push_back(boat);
	}

	INFO("Inited Scene.");
}

void Scene::Update() {

	_Renderer->BeforeDraw();
	_Renderer->Draw(_Renderables.data(), (int)_Renderables.size());
	_Renderer->AfterDraw();

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

void Scene::Destroy() {
	if (_Renderer != nullptr)
	{
		_Renderer->Release();
		free(_Renderer);
	}
}

RenderObject Scene::GenerateBoudingBox(const Mesh* mesh) {
	
	RenderObject obj;
	obj.material = ((Renderer*)_Renderer)->GetMaterial("DrawLine");

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

	((Renderer*)_Renderer)->LoadMesh("Bouding", boundingMesh);

	obj.mesh = ((Renderer*)_Renderer)->GetMesh("Bouding");

	return obj;
}
