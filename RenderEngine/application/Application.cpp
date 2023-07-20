#include "Application.hpp"

using namespace udon;

EngineLogger* GLOBAL_LOGGER = new EngineLogger();

Engine::Engine(){
    
    _window = nullptr;

}

Engine::~Engine(){
    Close();
}

bool Engine::Init(){

    _window = WsiWindow::GetInstance()->GetWindow();
    CHECK(_window);

    return true;
}


void Engine::Run(){
    while(1){
        std::cout << "Run.." << std::endl;
    }

}

void Engine::Close(){

    WsiWindow::DestoryWindow();

}


