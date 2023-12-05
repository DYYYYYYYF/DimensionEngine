#define USE_LOGGER

#include <iostream>
#include "../../application/Application.hpp"
#include "../../engine/EngineLogger.hpp"

#ifdef USE_LOGGER
static EngineLogger* GLOBAL_LOGGER = new EngineLogger();
#endif // !USE_LOGGER

int main(int argc, char* argv[]){

    ConfigFile config;
    config.LoadFile("../default/config.txt");
    config.SaveToFile();

    Application* pApp = new Application(&config);

    try {
        pApp->Init();
        pApp->Run();
        pApp->Close();
    } catch(const std::exception& e){
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    int a;
    std::cin >> a;

    return 0;
}
