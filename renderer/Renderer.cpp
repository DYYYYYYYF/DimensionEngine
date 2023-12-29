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

    // Default material
    CreateMaterial("Default", "shader/glsl/default_vert.spv", "shader/glsl/default_frag.spv");
   
    Material mat;
    CreatePipeline(mat, "shader/glsl/mesh_grid_vert.spv", "shader/glsl/mesh_grid_frag.spv", true);
    AddMaterial("Grid", mat);

    CreateDrawlinePipeline(mat, "shader/glsl/default_vert.spv", "shader/glsl/default_frag.spv");
    AddMaterial("Line", mat);

    // Models
    LoadTriangleMesh();

    // Compute test
    Material computeMaterial;
    CreateComputePipeline(computeMaterial, "shader/glsl/partical_solver_comp.spv");
    AddMaterial("Compute", computeMaterial);
    
    return true;
}

void Renderer::BeforeDraw(){
}

void Renderer::Draw(RenderObject* first, size_t count){
    _RendererImpl->DrawPerFrame(first, count, _Particals.data(), _Particals.size());

#ifdef DEBUG
    if (_Particals.size() > 0){
        size_t size = _Particals[0].GetParticalCount() * sizeof(ParticalData);
        ((VulkanRenderer*)_RendererImpl)->MemoryMap(_Particals[0].writeStorageBuffer, _Particals[0].writeData.data(), 0, size);
        std::cout << "Double: " << _Particals[0].writeData[3].position.x << " " << _Particals[0].writeData[3].position.y << " "
            << _Particals[0].writeData[3].position.z << " " << _Particals[0].writeData[3].position.w << std::endl;
    }
#endif
}

void Renderer::AfterDraw(){

}

void Renderer::CreatePipeline(Material& mat, const char* vert_shader, const char* frag_shader, bool alpha) {
    if (vert_shader == nullptr || vert_shader[0] == '\0' ||
        frag_shader == nullptr || frag_shader[0] == '\0') {
        return;
    }

    std::string vert = _RootDirection + std::string(vert_shader);
    if (vert.length() == 0) {
        return;
    }

    std::string frag = _RootDirection + std::string(frag_shader);
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

    std::string vert = _RootDirection + std::string(vert_shader);
    if (vert.length() == 0) {
        return;
    }

    std::string frag = _RootDirection + std::string(frag_shader);
    if (frag.length() == 0) {
        return;
    }

    ((VulkanRenderer*)_RendererImpl)->CreateDrawLinePipeline(mat, vert.data(), frag.data());
}
void Renderer::CreateComputePipeline(Material& mat, const char* comp_shader) {
    if (comp_shader == nullptr || comp_shader[0] == '\0') {
        return;
    }

    std::string comp = _RootDirection + std::string(comp_shader);
    if (comp.length() == 0) {
        return;
    }

    ((VulkanRenderer*)_RendererImpl)->CreateComputePipeline(mat, comp.data());
}

void Renderer::LoadMesh(const char* filename, const char* mesh_name) {

    if (filename == nullptr || filename[0] == '\0') {
        return;
    }

    std::string path = _RootDirection + std::string(filename);
    if (path.length() == 0) {
        return;
    }

    Mesh tempMesh;
    if (!tempMesh.LoadFromObj(path.data())) {
        return;
    }

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

    std::string path = _RootDirection + std::string(texture_path);
    if (path.length() == 0) {
        return;
    }

    Texture texture;

    renderer::LoadImageFromFile(*((VulkanRenderer*)_RendererImpl), path.data(), texture.image);
    ASSERT(texture.image.image)

    texture.imageView = ((VulkanRenderer*)_RendererImpl)->CreateImageView(vk::Format::eR8G8B8A8Srgb, texture.image.image, vk::ImageAspectFlagBits::eColor);
    AddTexture(filename, texture);
}

void Renderer::CreateMaterial(const char* filename, const char* vertShader, const char* fragShader) {
    Material material;
    CreatePipeline(material, vertShader, fragShader);
    material.material_name = filename;
    AddMaterial(filename, material);
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


void Renderer::LoadPartical(Particals partical){

    _Particals.push_back(partical);
    ((VulkanRenderer*)_RendererImpl)->BindBufferDescriptor(GetMaterial("Compute"), &_Particals[0]);
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

void Renderer::DrawPoint(Vector3 position, Vector3 color) {
    RenderObject point;
    Mesh pointMesh;

    pointMesh.vertices.resize(1);
    pointMesh.vertices[0].position = position;
    pointMesh.vertices[0].normal = { 0, 1, 0 };
    pointMesh.vertices[0].color = color;

    pointMesh.vertices[0].texCoord = { 0.0f, 0.0f };

    pointMesh.indices.resize(1);
    pointMesh.indices[0] = 0;

    LoadMesh("Point", pointMesh);
    Mesh* mesh = GetMesh("Point");
    if (mesh == nullptr) {
        WARN("Draw rectangle failed! Mesh %s is nullptr!", "Rectangle");
        return;
    }

    Material* material = GetMaterial("Point");
    if (material == nullptr) {
        WARN("Draw rectangle failed! Material %s is nullptr!", material->material_name.data());
        return;
    }

    point.SetMesh(mesh);
    point.SetMaterial(material);
    point.SetTranslate(position);

    _Renderables->push_back(point);
}

void Renderer::DrawLine(Vector3 p1, Vector3 p2, Vector3 color) {
    RenderObject line;
    Mesh lineMesh;
    lineMesh.vertices.resize(2);
    lineMesh.vertices[0].position = p1;
    lineMesh.vertices[0].normal = { 0, 1, 0 };
    lineMesh.vertices[0].color = color;

    lineMesh.vertices[1].position = p2;
    lineMesh.vertices[1].normal = { 0, 1, 0 };
    lineMesh.vertices[1].color = color;

    lineMesh.vertices[0].texCoord = { 0.0f, 0.0f };
    lineMesh.vertices[1].texCoord = { 1.0f, 1.0f };

    lineMesh.indices.resize(2);
    lineMesh.indices[0] = 0;
    lineMesh.indices[1] = 1;

    std::string name = "Line" + std::to_string(_Renderables->size());
    LoadMesh(name.data(), lineMesh);

    Mesh* mesh = GetMesh(name.data());
    if (mesh == nullptr) {
        WARN("Draw Line failed! Mesh %s is nullptr!", name.data());
        return;
    }

    Material* material = GetMaterial("Line");
    if (material == nullptr) {
        WARN("Draw Line failed! Material %s is nullptr!", material->material_name.data());
        return;
    }

    line.SetMesh(mesh);
    line.SetMaterial(material);
    _Renderables->push_back(line);
}

void Renderer::DrawRectangle(Vector3 position, Vector3 half_extent,
    Vector3 color, bool is_fill/* = true*/) {
    RenderObject rectangle;
    Mesh rectangleMesh;
    //make the array 6 vertices long
    rectangleMesh.vertices.resize(4);

    //vertex positions
    rectangleMesh.vertices[0].position = { half_extent.x,  0.f,  -half_extent.y };	//?Һ?
    rectangleMesh.vertices[1].position = { half_extent.x,  0.f,   half_extent.y };	//??ǰ
    rectangleMesh.vertices[2].position = { -half_extent.x, 0.f,   half_extent.y };	//??ǰ
    rectangleMesh.vertices[3].position = { -half_extent.x, 0.f,  -half_extent.y };	//????

    //vertex colors
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
    if (is_fill) {
        rectangleMesh.indices.resize(6);
        rectangleMesh.indices[0] = 0;
        rectangleMesh.indices[1] = 1;
        rectangleMesh.indices[2] = 2;
        rectangleMesh.indices[3] = 0;
        rectangleMesh.indices[4] = 2;
        rectangleMesh.indices[5] = 3;
    }
    else {
        rectangleMesh.indices.resize(5);
        rectangleMesh.indices[0] = 0;
        rectangleMesh.indices[1] = 1;
        rectangleMesh.indices[2] = 2;
        rectangleMesh.indices[3] = 3;
        rectangleMesh.indices[4] = 0;
    }

    std::string name = "Rectangle" + std::to_string(_Renderables->size());
    LoadMesh(name.data(), rectangleMesh);

    Mesh* mesh = GetMesh(name.data());
    if (mesh == nullptr) {
        WARN("Draw rectangle failed! Mesh %s is nullptr!", name.data());
        return;
    }

    Material* material = is_fill ? GetMaterial("Default") : GetMaterial("Line");
    if (material == nullptr) {
        WARN("Draw rectangle failed! Material %s is nullptr!", material->material_name.data());
        return;
    }

    rectangle.SetMesh(mesh);
    rectangle.SetMaterial(material);
    rectangle.SetTranslate(position);
    _Renderables->push_back(rectangle);
}

void Renderer::DrawCircle(Vector3 position, float radius, Vector3 color,
    bool is_fill /*= true*/, int side_count/* = 360*/) {
    RenderObject circle;

    double r = (360.0 / side_count) * 3.1415926 / 180;

    Mesh circleMesh;
    circleMesh.vertices.resize(side_count + 2);
    for (int i = 0; i < side_count + 1; ++i) {
        circleMesh.vertices[i].position = { cos(r * i) * radius, 0, sin(r * i) * radius };
        circleMesh.vertices[i].normal = { 0, 1, 0 };
        circleMesh.vertices[i].color = color;
        circleMesh.vertices[i].texCoord = { 1.0f / cos(r) * radius, 1.0f / sin(r) * radius };
    }

    circleMesh.vertices[side_count + 1].position = { 0, 0, 0 };
    circleMesh.vertices[side_count + 1].normal = { 0, 1, 0 };
    circleMesh.vertices[side_count + 1].color = color;
    circleMesh.vertices[side_count + 1].texCoord = { 0, 0 };

    std::string name = "Circle" + std::to_string(_Renderables->size());

    if (is_fill) {
        circleMesh.indices.resize(side_count * 3);
        for (int i = 0; i < 3 * side_count; ++i) {
            int t = i / 3;
            circleMesh.indices[i++] = t;
            circleMesh.indices[i++] = side_count + 1;
            circleMesh.indices[i] = t + 1;
        }
    } else {
        circleMesh.indices.resize(side_count * 2);
        for (int i = 0; i < 2 * side_count; ++i) {
            int t = i / 2;
            circleMesh.indices[i++] = t;
            circleMesh.indices[i] = t + 1;
        }
    }

    LoadMesh(name.data(), circleMesh);
    Mesh* mesh = GetMesh(name.data());
    if (mesh == nullptr) {
        WARN("Draw rectangle failed! Mesh %s is nullptr!", name.data());
        return;
    }

    Material* material = is_fill ? GetMaterial("Default") : GetMaterial("Line");
    if (material == nullptr) {
        WARN("Draw rectangle failed! Material %s is nullptr!", material->material_name.data());
        return;
    }

    circle.SetMesh(mesh);
    circle.SetMaterial(material);
    circle.SetTranslate(position);
    _Renderables->push_back(circle);
}
