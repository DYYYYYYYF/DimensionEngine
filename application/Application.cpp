#include "Application.hpp"
#include "Window.hpp"

Engine::Engine(){
    _window = nullptr;
    _Scene = nullptr;
}

Engine::~Engine(){
    _window = nullptr;
    _Scene = nullptr;
}

bool Engine::Init(){

    WsiWindow::SetWidth(1200);
    _window = WsiWindow::GetInstance()->GetWindow();
    ASSERT(_window);

    _Scene = new Scene();
    ASSERT(_Scene);
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


