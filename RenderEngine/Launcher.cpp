#include <iostream>
#include "Logger.hpp"
#include "engine/EngineLogger.hpp"


int main(int argc, char* argv[]){

    Log::Logger::getInstance()->open("bin/EngineLog.log");

    std::cout << "Launch Vulkan!" << std::endl;

    return 0;
}
