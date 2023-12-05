#include "Application.hpp"
#include "Window.hpp"

Application::Application(){
    _window = nullptr;
    _Scene = nullptr;
    _ConfigFile = nullptr;
}

Application::Application(ConfigFile* config) {
    _window = nullptr;
    _Scene = nullptr;
    _ConfigFile = config;
}

Application::~Application(){
    _window = nullptr;
    _Scene = nullptr;
}

bool Application::Init(){

    WsiWindow::SetWidth(1200);
    _window = WsiWindow::GetInstance()->GetWindow();
    ASSERT(_window);

    _Scene = new Scene();
    ASSERT(_Scene);
    _Scene->InitScene(_ConfigFile);

    return true;
}


void Application::Run(){

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

void Application::Close(){
    if (_Scene != nullptr) {
        _Scene->Destroy();
    }
    WsiWindow::DestoryWindow();

}


