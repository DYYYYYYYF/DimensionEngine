#pragma once
#include "Logger.hpp"

namespace udon {
#ifdef __DEBUG__
#define CHECK(expr) \
    if(!(expr)) { \
        FATAL(#expr "is null!"); \
        exit(-1); \
}
#else
#define CHECK(expr) 
#endif //ifdef DEBUG
}

