#include "Renderer.hpp"
#include <cstdlib>

Renderer::Renderer(){
    _Context = nullptr;
}

Renderer::~Renderer(){

}

bool Renderer::Init(){
    _Context = new VkContext();
    if (!_Context->InitContext()){
        DEBUG("Create Context Failed.");
        return false;
    }

    return true;
}

