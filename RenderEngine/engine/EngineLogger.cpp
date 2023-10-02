#include "EngineLogger.hpp"

using namespace udon;

#ifdef DEBUG
EngineLogger::EngineLogger(){
    // Get current path
    std::filesystem::path curPath = std::filesystem::current_path();
    curPath.append("bin/EngineLog.log");

    Log::Logger::getInstance()->open(curPath.c_str());
    Log::Logger::getInstance()->setMaxSize(1024000);

    INFO("Logger Init Success.");
}

#else

EngineLogger::EngineLogger(){
    // Get current path
    std::filesystem::path curPath = std::filesystem::current_path();
    curPath.append("bin/EngineLog.log");

    Log::Logger::getInstance()->open(curPath.c_str());
    INFO("Logger Init Success.");
}
#endif
