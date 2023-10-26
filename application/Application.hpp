#pragma once
#include "Window.hpp"
#include "utils/EngineUtils.hpp"
#include "../renderer/Renderer.hpp"

namespace udon{
class Engine{
public:
    Engine();
    virtual ~Engine();

    bool Init();
    void Run();
    void Close();

private:
    SDL_Window* _window;
    Renderer* _Renderer;
    int _FrameCount = 0;

};
} 
