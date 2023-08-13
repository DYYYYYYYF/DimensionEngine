#include "Application.hpp"
#include "Window.hpp"

using namespace udon;

EngineLogger* GLOBAL_LOGGER = new EngineLogger();

Engine::Engine(){
    
    _window = nullptr;
    _Renderer = nullptr;
}

Engine::~Engine(){
    Close();
}

bool Engine::Init(){

    _Renderer = new Renderer();    
    CHECK(_Renderer);
    _Renderer->Init();

    _window = WsiWindow::GetInstance()->GetWindow();
    CHECK(_window);

    return true;
}


void Engine::Run(){

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

    }
}

void Engine::Close(){

    WsiWindow::DestoryWindow();

}


