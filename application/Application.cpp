#include "Application.hpp"
#include "Window.hpp"

using namespace udon;

Engine::Engine(){
    
    _window = nullptr;
    _Renderer = nullptr;
}

Engine::~Engine(){
    Close();
}

bool Engine::Init(){

    _window = WsiWindow::GetInstance()->GetWindow();
    CHECK(_window);

    _Renderer = new Renderer();    
    CHECK(_Renderer);
    _Renderer->Init();

    return true;
}


void Engine::Run(){

    _Renderer->BeforeDraw();
    if (!_window){
        DEBUG("_window is null.");
        Close();
        return;
    }

    SDL_Event event;
    bool isQuit = false;
    while(!isQuit){
        if(SDL_PollEvent(&event)){
            if(event.type == SDL_QUIT){
                isQuit = true;
            }
        }

        _Renderer->Draw();
        _FrameCount++;
    }
}

void Engine::Close(){

    WsiWindow::DestoryWindow();

}


