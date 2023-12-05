#define USE_LOGGER

#include <iostream>
#include "../application/Application.hpp"
#include "../engine/EngineLogger.hpp"

#ifdef USE_LOGGER
static EngineLogger* GLOBAL_LOGGER = new EngineLogger();
#endif // !USE_LOGGER

int main(int argc, char* argv[]){

    Engine* pEngine = new Engine();

    try{
        pEngine->Init();
        pEngine->Run();
        pEngine->Close();
    } catch(const std::exception& e){
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return 0;
}
