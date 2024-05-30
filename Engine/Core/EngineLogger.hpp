#pragma once
#include "Platform/Platform.hpp"
#include <Logger.hpp>
#include <filesystem>

class EngineLogger{
public:
    EngineLogger();
    virtual ~EngineLogger(){}
};

// Logger
#ifdef LEVEL_DEBUG
#define ASSERT(expr) \
    if(!(expr)) { \
        UL_DEBUG(#expr "is null!"); \
        exit(-1); \
}
#else
#define ASSERT(expr) {}
#endif //ifdef DEBUG

#ifndef LOG_DEBUG
#define LOG_DEBUG(...) \
    Platform::PlatformConsoleWrite(__VA_ARGS__"\n", 3);  \
    UL_DEBUG(__VA_ARGS__);
#endif

#ifndef LOG_INFO
#define LOG_INFO(...) \
    Platform::PlatformConsoleWrite(__VA_ARGS__"\n", 5);  \
    UL_INFO(__VA_ARGS__);
#endif

#ifndef LOG_WARN
#define LOG_WARN(...) \
    Platform::PlatformConsoleWrite(__VA_ARGS__"\n", 2);  \
    UL_WARN(__VA_ARGS__);
#endif

#ifndef LOG_ERROR
#define LOG_ERROR(...) \
    Platform::PlatformConsoleWrite(__VA_ARGS__"\n", 1);  \
    UL_ERROR(__VA_ARGS__);
#endif

#ifndef LOG_FATAL
#define LOG_FATAL(...) \
    Platform::PlatformConsoleWrite(__VA_ARGS__"\n", 0);  \
    UL_FATAL(__VA_ARGS__);
#endif

#define CoreLog UL_INFO
