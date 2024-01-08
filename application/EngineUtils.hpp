#pragma once

#include "Logger.hpp"

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
