#pragma once
#include <Logger.hpp>
#include <filesystem>

#ifdef _DEBUG
#define ASSERT(expr) \
    if(!(expr)) { \
        FATAL(#expr "is null!"); \
        exit(-1); \
}
#else
#define ASSERT(expr) \
    if(!(expr)) { \
        FATAL(#expr "is null!"); \
        exit(-1); \
}
#endif //ifdef DEBUG

class EngineLogger{
public:
    EngineLogger();
    virtual ~EngineLogger(){}
};

