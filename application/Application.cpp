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

    // Init window from config
    std::string width = "";
    std::string aspect = "";
    if (_ConfigFile != nullptr){
        width = _ConfigFile->GetVal("WindowWidth");
        aspect = _ConfigFile->GetVal("WindowAspect");
    }

    WsiWindow::SetWidth(width.length() == 0 ? 800 : std::stoi(width));
    WsiWindow::SetAspect(aspect.length() == 0 ? (float)16 / 9 : std::stof(aspect));

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

    // Save window config
    if (_ConfigFile) {
        _ConfigFile->SetData("WindowWidth", std::to_string(WsiWindow::GetWidth()));
        _ConfigFile->SetData("WindowAspect", std::to_string(WsiWindow::GetAspect()));
    }

    if (_Scene != nullptr) {
        _Scene->Destroy();
    }
    WsiWindow::DestoryWindow();

}


