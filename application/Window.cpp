#include"Window.hpp"

using namespace udon;

int WsiWindow::_Height = 800;
int WsiWindow::_Width = 1200;
WsiWindow* WsiWindow::_WindowObj = nullptr;
SDL_Window* WsiWindow::_SDLWindow = nullptr;

WsiWindow::WsiWindow(int height, int width){
    _SDLWindow = nullptr;
    _WindowObj = nullptr;
    _Height = height;
    _Width = width;

    InitWindow();
}
WsiWindow::~WsiWindow(){}

bool WsiWindow::InitWindow(){

    if(SDL_Init(SDL_INIT_EVERYTHING) < 0) return false;

    _SDLWindow = SDL_CreateWindow("RenderEngine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
        _Width, _Height, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);

    CHECK(_SDLWindow);
    if(!_SDLWindow) return false;

    return true;
}

SDL_Window* WsiWindow::GetWindow(){
   return _SDLWindow;
}

WsiWindow* WsiWindow::GetInstance(){
    if(!_WindowObj){
        _WindowObj = new WsiWindow(400, 600);
    }

    CHECK(_WindowObj);
    return _WindowObj;
}

void WsiWindow::DestoryWindow(){
    
    if(!_SDLWindow) return;
    SDL_DestroyWindow(_SDLWindow);
    SDL_Quit();
}

void WsiWindow::GetWindowSize(int& w, int& h){
    w = _Width;
    h = _Height;
}
