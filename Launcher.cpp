#define USE_LOGGER
#define _DEBUG

#include <iostream>
#include "application/Application.hpp"
#include "../../engine/EngineLogger.hpp"

#ifdef USE_LOGGER
static engine::EngineLogger* GLOBAL_LOGGER = new engine::EngineLogger();
#endif // !USE_LOGGER

int main(int argc, char* argv[]){

    udon::Engine* Engine = new udon::Engine();

    try{
        Engine->Init();
        Engine->Run();
        Engine->Close();
    } catch(const std::exception& e){
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return 0;
}
