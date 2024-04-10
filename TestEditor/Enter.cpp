#if defined(_WIN32) && !defined(LEVEL_DEBUG)
#pragma comment( linker, "/subsystem:windows /entry:mainCRTStartup" )
#endif

#define USE_LOGGER
#include <iostream>
#include "../engine/platform/platform.hpp"

int main(int argc, char* argv[]) {

    SPlatformState state;
    PlatformStartup(&state, "Dimension Editor", 100, 100, 1200, 800);
    
    while (true) {
        PlatformPumpMessage(&state);

    }

    PlatformShutdown(&state);

    return 0;
}
