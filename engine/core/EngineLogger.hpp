#pragma once
#include <Logger.hpp>
#include <filesystem>

#ifdef LEVEL_DEBUG
#define ASSERT(expr) \
    if(!(expr)) { \
        UL_DEBUG(#expr "is null!"); \
        exit(-1); \
}
#else
#define ASSERT(expr) \
    if(!(expr)) { \
        UL_FATAL(#expr "is null!"); \
        exit(-1); \
}
#endif //ifdef DEBUG

#define CoreLog UL_INFO

class EngineLogger{
public:
    EngineLogger();
    virtual ~EngineLogger(){}
};

