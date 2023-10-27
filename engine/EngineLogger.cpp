#include "EngineLogger.hpp"

#ifdef __APPLE__
using namespace std::filesystem;
#elif _WIN32
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>
using namespace std::experimental::filesystem;
#endif

using namespace udon;

#ifdef DEBUG
EngineLogger::EngineLogger(){
    // Get current path
    path curPath = current_path();
    curPath.append("bin/EngineLog.log");

#ifdef __APPLE__
    Log::Logger::getInstance()->open(curPath.c_str());
#elif _WIN32
    Log::Logger::getInstance()->open(curPath.u8string());
#endif

    Log::Logger::getInstance()->setMaxSize(1024000);

    INFO("Logger Init Success.");
}

#else

EngineLogger::EngineLogger(){
    // Get current path
    std::filesystem::path curPath = std::filesystem::current_path();
    curPath.append("bin/EngineLog.log");

#ifdef __APPLE__
    Log::Logger::getInstance()->open(curPath.c_str());
#elif _WIN32
    Log::Logger::getInstance()->open(curPath.u8string());
#endif

    Log::Logger::getInstance()->open(curPath.c_str());
    Log::Logger::getInstance()->setMaxSize(1024000);

    INFO("Logger Init Success.");
}
#endif
