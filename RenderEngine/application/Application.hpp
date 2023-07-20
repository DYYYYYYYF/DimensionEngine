#pragma once
#include "Window.hpp"
#include "utils/EngineUtils.hpp"
#include "../vulkan/VkContext.hpp"

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

};
} 
