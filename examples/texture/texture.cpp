#if defined(_WIN32) && !defined(LEVEL_DEBUG)
#pragma comment( linker, "/subsystem:windows /entry:mainCRTStartup" )
#endif

#define USE_LOGGER
#include <iostream>
#include "../../application/Application.hpp"
#include "../../engine/core/EngineLogger.hpp"

#ifdef USE_LOGGER
static EngineLogger* GLOBAL_LOGGER = new EngineLogger();
#endif // !USE_LOGGER

int main(int argc, char* argv[]){

    ConfigFile config;
    config.LoadFile("../texture/config.conf");

    Application* pApp = new Application(&config);

    try {
        pApp->Init();
        pApp->Run();
        pApp->Close();
    } catch(const std::exception& e){
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    config.SaveToFile();

    return 0;
}
