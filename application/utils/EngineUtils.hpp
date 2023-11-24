#pragma once
#include "Logger.hpp"

namespace udon {
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
}

