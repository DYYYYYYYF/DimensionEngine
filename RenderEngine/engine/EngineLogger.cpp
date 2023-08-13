#include "EngineLogger.hpp"

using namespace udon;

#ifdef DEBUG
EngineLogger::EngineLogger(){
    Log::Logger::getInstance()->open("bin/EngineLog.log");
    DEBUG("Logger Init Success.");
}

#else
EngineLogger::EngineLogger(){
    Log::Logger::getInstance()->open("bin/EngineLog.log");
    INFO("Logger Init Success.");
}
#endif
