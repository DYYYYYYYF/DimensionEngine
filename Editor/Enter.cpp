#if defined(_WIN32) && !defined(LEVEL_DEBUG)
#pragma comment( linker, "/subsystem:windows /entry:mainCRTStartup" )
#endif

#include "Game.h"
#include "Entry.hpp"
#include <Platform/JsonObject.h>

// TODO: Remove
#include "Core/DMemory.hpp"

extern bool CreateGame(IGame* out_game) {
	// Create Game state
	File MaterialAsset(EDITOR_CONFIG_PATH);
	if (!MaterialAsset.IsExist()) {
		return false;
	}

	JsonObject Content = JsonObject(MaterialAsset.ReadBytes());

	out_game->SetWindowWidth(Content.ReadInt("Window.Width"));
	out_game->SetWindowHeight(Content.ReadInt("Window.Height"));
	out_game->SetWindowOffsetX(Content.ReadInt("Window.OffsetX"));
	out_game->SetWindowOffsetY(Content.ReadInt("Window.OffsetY"));
	out_game->SetApplicationName(Content.ReadString("Window.Title")); 

    return true;
}
