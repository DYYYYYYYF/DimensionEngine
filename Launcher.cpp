#include <iostream>
#include "application/Application.hpp"


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
