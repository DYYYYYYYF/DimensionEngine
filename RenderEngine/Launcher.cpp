#include <iostream>
#include "application/Application.hpp"


int main(int argc, char* argv[]){

    udon::Engine* Engine = new udon::Engine();

    Engine->Init();
    Engine->Run();
    Engine->Close();

    std::cout << "Launch Vulkan!" << std::endl;

    return 0;
}
