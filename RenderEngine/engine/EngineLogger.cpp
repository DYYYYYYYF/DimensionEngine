#include "EngineLogger.hpp"

using namespace udon;

EngineLogger::EngineLogger(){
    Log::Logger::getInstance()->open("bin/EngineLog.log");
    DEBUG("Logger Init Success.");
}
