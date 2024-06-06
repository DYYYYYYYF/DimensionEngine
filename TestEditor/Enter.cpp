#if defined(_WIN32) && !defined(LEVEL_DEBUG)
#pragma comment( linker, "/subsystem:windows /entry:mainCRTStartup" )
#endif

#include "Game.hpp"
#include "Entry.hpp"

// TODO: Remove
#include "Core/DMemory.hpp"

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
    out_game->state = Memory::Allocate(sizeof(SGameState), MemoryType::eMemory_Type_Game);
    
    return true;
}
