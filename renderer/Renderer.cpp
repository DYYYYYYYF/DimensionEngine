#include "Renderer.hpp"
#include <cstdlib>

using namespace renderer;

Renderer::Renderer(){
    _RendererImpl = new VulkanRenderer();
    if (_RendererImpl == nullptr) {
        FATAL("Create Renderer failed.");
    }

    _ConfigFile = nullptr;
}

Renderer::~Renderer(){
    _RendererImpl = nullptr;
    _ConfigFile = nullptr;
}

bool Renderer::Init(){

    _RendererImpl->Init();

    // Models
    LoadMesh("asset/model/room.obj", "room");
    LoadMesh("asset/model/ball.obj", "ball");
    LoadMesh("asset/model/CornellBox.obj", "CornellBox");
    LoadMesh("asset/model/sponza.obj", "sponza");
    LoadMesh("asset/model/bunny.obj", "Bunny");
    LoadMesh("asset/obj/wooden_boat/Boat.obj", "Boat");
    LoadTriangleMesh();
    LoadRectangleMesh();

    // Textures
    LoadTexture("room", "asset/texture/room.png");
    LoadTexture("wooden", "asset/obj/wooden_boat/BaseColor.png");

    // Materials
    const char* defaultVertShader = "shader/glsl/default_vert.spv";
    const char* defaultFragShader = "shader/glsl/default_frag.spv";
    const char* meshVertShader = "shader/glsl/texture_mesh_vert.spv";
    const char* meshFragShader = "shader/glsl/texture_mesh_frag.spv";
    const char* meshFloorVertShader = "shader/glsl/mesh_grid_vert.spv";
    const char* meshFloorFragShader = "shader/glsl/mesh_grid_frag.spv";

    ((VulkanRenderer*)_RendererImpl)->UseTextureSet(false);
    Material deafaultMaterial;
    CreatePipeline(deafaultMaterial, defaultVertShader, defaultFragShader);
    AddMaterial("Default", deafaultMaterial);

    Material drawLineMaterial;
    CreateDrawlinePipeline(drawLineMaterial, defaultVertShader, defaultFragShader);
    AddMaterial("DrawLine", drawLineMaterial);

    ((VulkanRenderer*)_RendererImpl)->UseTextureSet(true);
    Material deafaultMeshMaterial;
    CreatePipeline(deafaultMeshMaterial, meshVertShader, meshFragShader);
    AddMaterial("Texture", deafaultMeshMaterial);

    Material deafaultFloorMaterial;
    CreatePipeline(deafaultFloorMaterial, meshFloorVertShader, meshFloorFragShader, true);
    AddMaterial("Floor", deafaultFloorMaterial);

    ((VulkanRenderer*)_RendererImpl)->BindTextureDescriptor(GetMaterial("Texture"), GetTexture("room"));

    // Compute test
    Material computeMaterial;
    CreateComputePipeline(computeMaterial, "shader/glsl/partical_solver_comp.spv");
    AddMaterial("Compute", computeMaterial);
    _Partical.SetPartialCount(256);
    _Partical.SetMaterial(GetMaterial("Compute"));
    ((VulkanRenderer*)_RendererImpl)->BindBufferDescriptor(GetMaterial("Compute"), &_Partical);

    return true;
}

void Renderer::BeforeDraw(){
}

void Renderer::Draw(RenderObject* first, int count){
    _RendererImpl->DrawPerFrame(first, count, &_Partical, 1);

#ifdef _DEBUG_
    size_t size = _Partical.GetParticalCount() * sizeof(ParticalData);
    ((VulkanRenderer*)_RendererImpl)->MemoryMap(_Partical.writeStorageBuffer, _Partical.writeData.data(), 0, size);
    std::cout << "after: " << _Partical.writeData[3].position.x << " " << _Partical.writeData[3].position.y << " "
        << _Partical.writeData[3].position.z << " " << _Partical.writeData[3].position.w << std::endl;
#endif
}

void Renderer::AfterDraw(){

}

void Renderer::CreatePipeline(Material& mat, const char* vert_shader, const char* frag_shader, bool alpha) {
    if (vert_shader == nullptr || vert_shader[0] == '\0' ||
        frag_shader == nullptr || frag_shader[0] == '\0') {
        return;
    }

    std::string vert = _PreFilePath + std::string(vert_shader);
    if (vert.length() == 0) {
        return;
    }

    std::string frag = _PreFilePath + std::string(frag_shader);
    if (frag.length() == 0) {
        return;
    }

    _RendererImpl->CreatePipeline(mat, vert.data(), frag.data(), alpha);
}

void Renderer::CreateDrawlinePipeline(Material& mat, const char* vert_shader, const char* frag_shader) {
    if (vert_shader == nullptr || vert_shader[0] == '\0' ||
        frag_shader == nullptr || frag_shader[0] == '\0') {
        return;
    }

    std::string vert = _PreFilePath + std::string(vert_shader);
    if (vert.length() == 0) {
        return;
    }

    std::string frag = _PreFilePath + std::string(frag_shader);
    if (frag.length() == 0) {
        return;
    }

    ((VulkanRenderer*)_RendererImpl)->CreateDrawLinePipeline(mat, vert.data(), frag.data());
}
void Renderer::CreateComputePipeline(Material& mat, const char* comp_shader) {
    if (comp_shader == nullptr || comp_shader[0] == '\0') {
        return;
    }

    std::string comp = _PreFilePath + std::string(comp_shader);
    if (comp.length() == 0) {
        return;
    }

    ((VulkanRenderer*)_RendererImpl)->CreateComputePipeline(mat, comp.data());
}

void Renderer::LoadMesh(const char* filename, const char* mesh_name) {

    if (filename == nullptr || filename[0] == '\0') {
        return;
    }

    std::string path = _PreFilePath + std::string(filename);
    if (path.length() == 0) {
        return;
    }

    Mesh tempMesh;
    tempMesh.LoadFromObj(path.data());
    _RendererImpl->UpLoadMeshes(tempMesh);
    AddMesh(mesh_name, tempMesh);
}

void Renderer::LoadMesh(const char* filename, Mesh& mesh) {
    _RendererImpl->UpLoadMeshes(mesh);
    AddMesh(filename, mesh);
}

void Renderer::LoadTexture(const char* filename, const char* texture_path) {

    if (texture_path == nullptr || texture_path[0] == '\0') {
        return;
    }

    std::string path = _PreFilePath + std::string(texture_path);
    if (path.length() == 0) {
        return;
    }

    Texture texture;

    renderer::LoadImageFromFile(*((VulkanRenderer*)_RendererImpl), path.data(), texture.image);
    ASSERT(texture.image.image)

    texture.imageView = ((VulkanRenderer*)_RendererImpl)->CreateImageView(vk::Format::eR8G8B8A8Srgb, texture.image.image, vk::ImageAspectFlagBits::eColor);
    AddTexture(filename, texture);
}

void Renderer::UpdateViewMat(glm::mat4 view_matrix){
    ((VulkanRenderer*)_RendererImpl)->UpdatePushConstants(view_matrix);
    ((VulkanRenderer*)_RendererImpl)->UpdateUniformBuffer();
    ((VulkanRenderer*)_RendererImpl)->UpdateDynamicBuffer();
}

void Renderer::Release() {
    if (_RendererImpl != nullptr) {
        ReleaseMeshes();
        ReleaseMaterials();
        ReleaseTextures();
        ReleaseParticals();
        
        _RendererImpl->Release();
        free(_RendererImpl);
    }
}

Material* Renderer::GetMaterial(const std::string& name) {
    //search for the object, and return nullptr if not found
    auto it = _Materials.find(name);
    if (it == _Materials.end()) {
        return nullptr;
    }
    else {
        return &(*it).second;
    }
}

Mesh* Renderer::GetMesh(const std::string& name) {
    auto it = _Meshes.find(name);
    if (it == _Meshes.end()) {
        return nullptr;
    }
    else {
        return &(*it).second;
    }
}

Texture* Renderer::GetTexture(const std::string& name) {
    auto it = _Textures.find(name);
    if (it == _Textures.end()) {
        return nullptr;
    }
    else {
        return &(*it).second;
    }
}

void Renderer::LoadTriangleMesh() {

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

	LoadMesh("Triangle", triganleMesh);

	INFO("Loaded Triangle");
}

void Renderer::LoadRectangleMesh() {
	Mesh rectangleMesh;
	//make the array 6 vertices long
	rectangleMesh.vertices.resize(4);

	//vertex positions
	rectangleMesh.vertices[0].position = { 50.f, 0.f,  -50.f };	//?Һ?
	rectangleMesh.vertices[1].position = { 50.f, 0.f,   50.f };	//??ǰ
	rectangleMesh.vertices[2].position = { -50.f, 0.f,   50.f };	//??ǰ
	rectangleMesh.vertices[3].position = { -50.f, 0.f,  -50.f };	//????

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

	LoadMesh("Rectangle",  rectangleMesh);

	INFO("Loaded Rectangle");
}
