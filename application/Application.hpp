#pragma once
#include "Window.hpp"
#include "../engine/Scene.hpp"

using namespace renderer;

class Application{
public:
    Application();
    Application(ConfigFile* config);
    virtual ~Application();

    bool Init();
    void Run();
    void Close();

private:
    GLFWwindow* _window;
    Scene* _Scene;
    ConfigFile* _ConfigFile;
    int _FrameCount = 0;

};
