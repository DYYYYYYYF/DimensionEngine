#include "EngineLogger.hpp"

using namespace udon;

#ifdef DEBUG
EngineLogger::EngineLogger(){
    // Get current path
    std::filesystem::path curPath = std::filesystem::current_path();
    curPath.append("bin/EngineLog.log");

    Log::Logger::getInstance()->open(curPath.c_str());
    DEBUG("Logger Init Success.");
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
