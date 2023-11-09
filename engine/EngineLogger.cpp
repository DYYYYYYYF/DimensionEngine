#include "EngineLogger.hpp"

#ifdef __APPLE__
using namespace std::filesystem;
#elif _WIN32
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>
using namespace std::experimental::filesystem;
#endif

using namespace engine;

#ifdef DEBUG
EngineLogger::EngineLogger(){
    // Get current path
    path curPath = current_path();
    curPath.append("EngineLog");

#ifdef __APPLE__
    Log::Logger::getInstance()->open(curPath.c_str(), std::ios_base::ate);
#elif _WIN32
    Log::Logger::getInstance()->open(curPath.u8string(), std::ios_base::ate);
#endif

    Log::Logger::getInstance()->setMaxSize(1024000);
    INFO("Logger Init Success.");
    INFO("Mode: Debug.");
}

#else

EngineLogger::EngineLogger(){
    // Get current path
    std::filesystem::path curPath = std::filesystem::current_path();
    curPath.append("EngineLog");

#ifdef __APPLE__
    Log::Logger::getInstance()->open(curPath.c_str(), std::ios_base::ate);
#elif _WIN32
    Log::Logger::getInstance()->open(curPath.u8string(), std::ios_base::ate);
#endif

    Log::Logger::getInstance()->setMaxSize(1024000);

    INFO("Logger Init Success.");
    INFO("Mode: Release.");
}
#endif
