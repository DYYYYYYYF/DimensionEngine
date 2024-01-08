#include "Application.hpp"

Application::Application() : m_nFPS(0), m_nLimitFPS(60), m_bLimitFPS(true){
    _window = nullptr;
    _Scene = nullptr;
    _ConfigFile = nullptr;
}

Application::Application(ConfigFile* config) {
    Application();
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

    WsiWindow::SetWidth(width.length() == 0 ? 1200 : std::stoi(width));
    WsiWindow::SetAspect(aspect.length() == 0 ? (float)16 / 9 : std::stof(aspect));

    _window = WsiWindow::GetInstance()->GetWindow();
    ASSERT(_window);

    _Scene = new Scene();
    ASSERT(_Scene);
    _Scene->InitScene(_ConfigFile);

#ifdef _WIN32
    QueryPerformanceFrequency(&_Freq);
    QueryPerformanceCounter(&_Time);
#elif __APPLE__

#endif
    SetLimitFPS(144);

    return true;
}


void Application::Run(){

    if (!_window){
        DEBUG("_window is null.");
        Close();
        return;
    }

    while (!glfwWindowShouldClose(_window)) {

        // FPS
#ifdef _WIN32
        // ms
        time_t start = clock();
        time_t end = clock();

        // us
        LARGE_INTEGER CurrentTime;
        QueryPerformanceCounter(&CurrentTime);
        double dt = (CurrentTime.QuadPart - _Time.QuadPart) / (double)_Freq.QuadPart;
        _Time = CurrentTime;

        if (dt < 1.0 / m_nLimitFPS && m_bLimitFPS) {
            double waitTime = 1.0 / m_nLimitFPS - dt;
            Sleep(waitTime * 1000);
        }

        GetCurrentFPS(dt);
#elif __APPLE__

#endif

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


