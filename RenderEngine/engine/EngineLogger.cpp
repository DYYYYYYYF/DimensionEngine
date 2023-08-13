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
    Log::Logger::getInstance()->open("bin/EngineLog.log");
    INFO("Logger Init Success.");
}
#endif
