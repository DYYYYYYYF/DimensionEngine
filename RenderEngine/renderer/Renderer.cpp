#include "Renderer.hpp"

Renderer::Renderer(){
    _Context = nullptr;
}

Renderer::~Renderer(){

}

bool Renderer::Init(){
    _Context = new VkContext();
    bool res = _Context->InitInstance();

    return res;
}

