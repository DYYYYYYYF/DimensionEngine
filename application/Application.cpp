#include "Application.hpp"
#include "Window.hpp"

using namespace udon;
using namespace engine;

Engine::Engine(){
    _window = nullptr;
    _Scene = nullptr;
}

Engine::~Engine(){
    _window = nullptr;
    _Scene = nullptr;
}

bool Engine::Init(){

    _window = WsiWindow::GetInstance()->GetWindow();
    CHECK(_window);

    _Scene = new Scene();
    CHECK(_Scene);
    _Scene->InitScene();

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
            if(event.type == SDL_QUIT || event.key.keysym.sym == SDLK_ESCAPE){
                isQuit = true;
            } else{
                _Scene->UpdatePosition(event);
            }
        }

        _Scene->Update();
        _FrameCount++;
    }
}

void Engine::Close(){
    if (_Scene != nullptr) {
        _Scene->Destroy();
    }
    WsiWindow::DestoryWindow();

}


