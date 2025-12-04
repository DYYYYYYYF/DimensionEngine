#if defined(_WIN32) && !defined(LEVEL_DEBUG)
#pragma comment( linker, "/subsystem:windows /entry:mainCRTStartup" )
#endif

#include "Game.hpp"
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

	out_game->SetWindowWidth(Content.Get("Window").Get("Width").GetInt());
	out_game->SetWindowHeight(Content.Get("Window").Get("Height").GetInt()); 
	out_game->SetWindowOffsetX(Content.Get("Window").Get("OffsetX").GetInt()); 
	out_game->SetWindowOffsetY(Content.Get("Window").Get("OffsetY").GetInt()); 
	out_game->SetApplicationName(Content.Get("Window").Get("Title").GetString()); 

    return true;
}
