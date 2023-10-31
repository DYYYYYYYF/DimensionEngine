#pragma once
#include "Logger.hpp"

namespace udon {
#ifdef _DEBUG
#define CHECK(expr) \
    if(!(expr)) { \
        FATAL(#expr "is null!"); \
        exit(-1); \
}
#else
#define CHECK(expr) \
        if(!(expr)) { \
            FATAL(#expr " IS NULL OR INVALID!"); \
            exit(-1); \
    }
#endif //ifdef DEBUG
}

