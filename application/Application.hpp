#pragma once
#include "Window.hpp"
#include "utils/EngineUtils.hpp"

#include "../engine/Scene.hpp"
#include "../audio/AudioContext.hpp"

using namespace renderer;

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
    AudioContext* _AudioContext;
    int _FrameCount = 0;

};
