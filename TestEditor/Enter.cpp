#if defined(_WIN32) && !defined(LEVEL_DEBUG)
#pragma comment( linker, "/subsystem:windows /entry:mainCRTStartup" )
#endif

#include "Game.hpp"
#include "../engine/Entry.hpp"

// TODO: Remove
#include <iostream>
#include "../engine/platform/platform.hpp"

extern bool CreateGame(SGame* out_game) {

    out_game->app_config.start_x = 100;
    out_game->app_config.start_y = 100;
    out_game->app_config.start_width = 1280;
    out_game->app_config.start_height = 720;
    out_game->app_config.name = "Dimension Editor";

    out_game->update = GameUpdate;
    out_game->render = GameRender;
    out_game->initialize = GameInitialize;
    out_game->on_resize = GameOnResize;

    // Create Game state
    out_game->state = PlatformAllocate(sizeof(SGameState), false);
    
    return true;
}