#include "Renderer.hpp"
#include "VulkanRenderer.hpp"
#include "renderer/interface/IRendererImpl.hpp"
#include <cstdlib>

using namespace renderer;

Renderer::Renderer(){
    _RendererImpl = new VulkanRenderer();
    if (_RendererImpl == nullptr) {
        FATAL("Create Renderer failed.");
    }
}

Renderer::~Renderer(){
    _RendererImpl = nullptr;
}

bool Renderer::Init(){
    _RendererImpl->Init();
    return true;
}

void Renderer::CreatePipeline(Material& mat) {
    _RendererImpl->CreatePipeline(mat);
}


void Renderer::BeforeDraw(){

}

void Renderer::Draw(RenderObject* first, int count){
    _RendererImpl->DrawPerFrame(first, count);
}

void Renderer::AfterDraw(){

}

void Renderer::UploadMeshes(Mesh& mesh) {
    _RendererImpl->UpLoadMeshes(mesh);
}

void Renderer::UpdateViewMat(glm::mat4 view_matrix){
    ((VulkanRenderer*)_RendererImpl)->UpdatePushConstants(view_matrix);
    ((VulkanRenderer*)_RendererImpl)->UpdateUniformBuffer();
    ((VulkanRenderer*)_RendererImpl)->UpdateDynamicBuffer();
}

void Renderer::Release() {
    if (_RendererImpl != nullptr) {
        _RendererImpl->Release();
        free(_RendererImpl);
    }
}
