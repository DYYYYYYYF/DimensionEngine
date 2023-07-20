#pragma once
#include <SDL.h>
#include <SDL_vulkan.h>
#include "utils/EngineUtils.hpp"

namespace udon {
class WsiWindow{
public:
    static SDL_Window* GetWindow();
    static WsiWindow* GetInstance();
    static void DestoryWindow();
    virtual ~WsiWindow();

private:
    WsiWindow(){}
    WsiWindow(int width, int height);
    static bool InitWindow();

private:
    static int _Height;
    static int _Width;
    static SDL_Window* _SDLWindow;

    static WsiWindow* _WindowObj;
};
}
