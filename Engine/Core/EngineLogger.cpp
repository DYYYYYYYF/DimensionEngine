#include "EngineLogger.hpp"

#ifdef __APPLE__
using namespace std::filesystem;
#elif _WIN32
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>
using namespace std::experimental::filesystem;
#endif

EngineLogger::EngineLogger(){
    // Get current path
    path curPath = current_path();
    curPath.append("EngineLog");

#ifdef __APPLE__
    Log::Logger::getInstance()->open(curPath.c_str(), std::ios_base::ate);
#elif _WIN32
    Log::Logger::getInstance()->open(curPath.u8string(), std::ios_base::ate);
#endif

    Log::Logger::Level LogLevel;
#ifdef LEVEL_DEBUG
    LogLevel = Log::Logger::Level::DEBUG;
    Log::Logger::getInstance()->SetMode(Log::Logger::eMode_Complex);
#else
    LogLevel = Log::Logger::Level::INFO;
	Log::Logger::getInstance()->SetMode(Log::Logger::eMode_Simple);
#endif

    Log::Logger::getInstance()->setMaxSize(1024000);
    Log::Logger::getInstance()->setLevel(LogLevel);

    LOG_INFO("Logger Init Success.");
    LOG_INFO("Mode: Debug.");

}

