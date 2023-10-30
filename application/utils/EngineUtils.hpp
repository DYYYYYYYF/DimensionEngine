#pragma once
#include "Logger.hpp"
#include "../../engine/EngineLogger.hpp"

namespace udon {

static EngineLogger* GLOBAL_LOGGER = new EngineLogger();

#ifdef DEBUG
    #define CHECK(expr) \
        if(!(expr)) { \
            DEBUG(#expr "is null!"); \
            exit(-1); \
    }
#else
    #define CHECK(expr)
#endif //ifdef DEBUG
}

