#include "Application.hpp"
#include "Window.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

using namespace udon;
using namespace engine;

Engine::Engine(){
    _window = nullptr;
    _Scene = nullptr;
}

Engine::~Engine(){
    _window = nullptr;
    _Scene = nullptr;
}

bool Engine::Init(){

    _window = WsiWindow::GetInstance()->GetWindow();
    CHECK(_window);

    _Scene = new Scene();
    CHECK(_Scene);
    _Scene->InitScene();

    return true;
}


void Engine::Run(){

    if (!_window){
        DEBUG("_window is null.");
        Close();
        return;
    }

    while (!glfwWindowShouldClose(_window)) {
        glfwPollEvents();

        _Scene->UpdatePosition(_window);
        _Scene->Update();
        
        _FrameCount++;
    }
}

void Engine::Close(){
    if (_Scene != nullptr) {
        _Scene->Destroy();
    }
    WsiWindow::DestoryWindow();

}


