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
	JSONReader JsonReader(EDITOR_CONFIG_PATH);
	out_game->SetWindowWidth(JsonReader.ReadPropertyInt("Window.Width"));
	out_game->SetWindowHeight(JsonReader.ReadPropertyInt("Window.Height"));
	out_game->SetWindowOffsetX(JsonReader.ReadPropertyInt("Window.OffsetX"));
	out_game->SetWindowOffsetY(JsonReader.ReadPropertyInt("Window.OffsetY"));
	out_game->SetApplicationName(JsonReader.ReadPropertyString("Application"));

    return true;
}
