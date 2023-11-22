#include "Application.hpp"
#include "Window.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

Engine::Engine(){
    _window = nullptr;
    _Scene = nullptr;
    _AudioContext = nullptr;
}

Engine::~Engine(){
    _window = nullptr;
    _Scene = nullptr;
    _AudioContext = nullptr;
}

bool Engine::Init(){

    WsiWindow::SetWidth(800);
    _window = WsiWindow::GetInstance()->GetWindow();
    CHECK(_window);

    _Scene = new Scene();
    CHECK(_Scene);
    _Scene->InitScene();

    _AudioContext = new AudioContext();
    _AudioContext->InitAudio();

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


