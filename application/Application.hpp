#pragma once
#include "Window.hpp"
#include "utils/EngineUtils.hpp"
#include "../engine/Scene.hpp"

using namespace renderer;
using namespace engine;

namespace udon{
class Engine{
public:
    Engine();
    virtual ~Engine();

    bool Init();
    void Run();
    void Close();

private:
    GLFWwindow* _window;
    Scene* _Scene;
    int _FrameCount = 0;

};
} 
