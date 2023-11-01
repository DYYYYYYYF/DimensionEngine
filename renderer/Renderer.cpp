#include "Renderer.hpp"
#include "VulkanRenderer.hpp"
#include "renderer/interface/IRendererImpl.hpp"
#include <cstdlib>

using namespace renderer;

Renderer::Renderer(){
    _Renderer = new VulkanRenderer();
    if (_Renderer == nullptr) {
        FATAL("Create Renderer failed.");
    }
}

Renderer::~Renderer(){
    if (_Renderer != nullptr){
        _Renderer->Release();
        _Renderer = nullptr;
    }
}

bool Renderer::Init(){
    _Renderer->Init();
    return true;
}

void Renderer::BeforeDraw() {
    _Renderer->CreatePipeline();
}

void Renderer::Draw(){
    ((VulkanRenderer*)_Renderer)->AddCount();
    _Renderer->DrawPerFrame();
}
