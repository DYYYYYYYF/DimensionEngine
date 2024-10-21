#if defined(_WIN32) && !defined(LEVEL_DEBUG)
#pragma comment( linker, "/subsystem:windows /entry:mainCRTStartup" )
#endif

#include "Game.hpp"
#include "Entry.hpp"

// TODO: Remove
#include "Core/DMemory.hpp"

extern bool CreateGame(IGame* out_game) {
	// Create Game state
    out_game->AppConfig.start_x = 100;
    out_game->AppConfig.start_y = 100;
    out_game->AppConfig.start_width = 1280;
    out_game->AppConfig.start_height = 720;
    out_game->AppConfig.name = "Dimension Editor";

    return true;
}
