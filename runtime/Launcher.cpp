#if defined(_WIN32) && !defined(LEVEL_DEBUG)
#pragma comment( linker, "/subsystem:windows /entry:mainCRTStartup" )
#endif

#define USE_LOGGER

#include <iostream>
#include "../application/Application.hpp"

int main(int argc, char* argv[]){

    Application* pApp = new Application();

    try{
        pApp->Init();
        pApp->Run();
        pApp->Close();
    } catch(const std::exception& e){
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return 0;
}
