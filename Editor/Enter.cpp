#if defined(_WIN32) && !defined(LEVEL_DEBUG)
#pragma comment( linker, "/subsystem:windows /entry:mainCRTStartup" )
#endif

#include "Game.hpp"
#include "Entry.hpp"

// TODO: Remove
#include "Core/DMemory.hpp"
#include "Utils/JSONReader.h"

extern bool CreateGame(IGame* out_game) {
	// Create Game state
	JSONReader JsonReader(std::string(ROOT_PATH) + "/Config/EngineConfig.json");
    out_game->AppConfig.start_width = JsonReader.ReadPropertyInt("Width");
	out_game->AppConfig.start_height = JsonReader.ReadPropertyInt("Height");
	out_game->AppConfig.start_x = JsonReader.ReadPropertyInt("OffsetX");
	out_game->AppConfig.start_y = JsonReader.ReadPropertyInt("OffsetY");
	out_game->AppConfig.name = JsonReader.ReadPropertyString("Application");

    return true;
}
