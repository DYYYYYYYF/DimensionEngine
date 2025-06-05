#if defined(_WIN32) && !defined(LEVEL_DEBUG)
#pragma comment( linker, "/subsystem:windows /entry:mainCRTStartup" )
#endif

#include "Game.hpp"
#include "Entry.hpp"

// TODO: Remove
#include "Core/DMemory.hpp"
#include "Utils/YAMLReader.h"

extern bool CreateGame(IGame* out_game) {
	// Create Game state
	YAMLReader YamlReader(EDITOR_CONFIG_PATH);
	out_game->SetWindowWidth(YamlReader.ReadPropertyInt("Window.Width"));
	out_game->SetWindowHeight(YamlReader.ReadPropertyInt("Window.Height"));
	out_game->SetWindowOffsetX(YamlReader.ReadPropertyInt("Window.OffsetX"));
	out_game->SetWindowOffsetY(YamlReader.ReadPropertyInt("Window.OffsetY"));
	out_game->SetApplicationName(YamlReader.ReadPropertyString("Window.Title"));

    return true;
}
