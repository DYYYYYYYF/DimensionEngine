#pragma once
#include "Logger.hpp"
#include "../../engine/EngineLogger.hpp"

namespace udon {

#define CHECK(expr) \
    if(!(expr)) { \
        DEBUG(#expr "is null!"); \
        exit(-1); \
}

}

