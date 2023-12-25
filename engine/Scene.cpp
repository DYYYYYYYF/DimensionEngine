#include "Scene.hpp"

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
	((Renderer*)_Renderer)->SetRenderObjects(&_Renderables);

	LoadRenderObjFromConfig(config);

	// Default draw floor
	_Renderer->DrawRectangle({ 0, 0, 0 }, { 6, 6, 0 }, { 1, 1, 1 }, false);

	if (config != nullptr){
		RenderObject floor;
		Mesh* rectMesh = ((Renderer*)_Renderer)->GetMesh("Rectangle1");
		Material* floorMaterial = ((Renderer*)_Renderer)->GetMaterial("Grid");
		if (rectMesh != nullptr && floorMaterial != nullptr) {
			floor.SetMesh(rectMesh);
			floor.SetMaterial(floorMaterial);
			floor.SetScale(0.9f);
			_Renderables.push_back(floor);
		}
	} else {
		_Renderer->DrawCircle({ 2, 0, 0 }, 10.0f, { 1,1,1 }, true, 360);
		_Renderer->DrawCircle({ 2, 0, 0 }, 9.0f, { 1,1,1 }, true, 10);
		;
		for (int i = -1; i <= 1; ++i) {
			for (int j = 1; j >= -1; --j) {
				if (i == 0 || j == 0) continue;
				_Renderer->DrawLine({ i * 6, 5, j * 6 }, { i * 6, -5, j * 6 }, { 1, 0, 1 });
			}
		}
	}

	Particals partical;
	partical.SetPartialCount(256);
	partical.SetMaterial(((Renderer*)_Renderer)->GetMaterial("Compute"));
	(((Renderer*)_Renderer)->LoadPartical(partical));

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
			// keyBoardScroll.rotateX -= glm::dot(glm::vec3(0, 0, x), glm::vec3(0, 0, y)) * 0.01;
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

void Scene::LoadRenderObjFromConfig(ConfigFile* config) {
	if (config == nullptr) {
		return;
	}

	const std::vector<ModelFile>& modelFiles = config->GetModelFiles();
	_Renderables.resize(modelFiles.size());

	int index = 0;
	for (const auto& modelfile : modelFiles) {
		RenderObject& obj = _Renderables[index];
		std::string modelFile = modelfile.GetModelFile();
		std::string textureFile = modelfile.GetTextureFile();

		// Model File
		size_t startPos = modelFile.find_last_of("/");
		size_t endPos = modelFile.find_last_of(".") - 1;
		std::string modelName = modelFile.substr(startPos + 1, endPos - startPos);
		Mesh* mesh = ((Renderer*)_Renderer)->GetMesh(modelName);
		if (mesh == nullptr) {
			((Renderer*)_Renderer)->LoadMesh(modelFile.data(), modelName.data());
			mesh = ((Renderer*)_Renderer)->GetMesh(modelName);
		}

		// Set enable texture
		((Renderer*)_Renderer)->SetEnableTexture(textureFile.length() != 0);

		// Shader File
		std::string vertFile = modelfile.GetVertFile();
		std::string fragFile = modelfile.GetFragFile();

		startPos = vertFile.find_last_of("/");
		endPos = vertFile.find_last_of("_") - 1;
		std::string materialName = vertFile.substr(startPos + 1, endPos - startPos);

		Material* material = ((Renderer*)_Renderer)->GetMaterial(materialName);
		if (material == nullptr) {
			((Renderer*)_Renderer)->CreateMaterial(materialName.data(), vertFile.data(), fragFile.data());
			material = ((Renderer*)_Renderer)->GetMaterial(materialName);
		}

		//Texture
		if (textureFile.length() != 0) {
			startPos = textureFile.find_last_of("/");
			endPos = textureFile.find_last_of(".") - 1;
			std::string textureName = textureFile.substr(startPos + 1, endPos - startPos);
			Texture* texture = ((Renderer*)_Renderer)->GetTexture(textureName);
			if (texture == nullptr) {
				((Renderer*)_Renderer)->LoadTexture(textureName.data(), textureFile.data());
				texture = ((Renderer*)_Renderer)->GetTexture(textureName);
			}

			((Renderer*)_Renderer)->BindTexture(material, textureName.data());
		}

		obj.SetMesh(mesh);
		obj.SetMaterial(material);
		obj.SetTranslate({ 0, 0, 0 });

		++index;
	}
}

RenderObject Scene::GenerateBoundingBox(const Mesh* mesh) {
	
	RenderObject obj;
	obj.SetMaterial(((Renderer*)_Renderer)->GetMaterial("DrawLine"));

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

	obj.SetMesh(((Renderer*)_Renderer)->GetMesh("Bouding"));

	return obj;
}
