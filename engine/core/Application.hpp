#pragma once

#include "EngineLogger.hpp"
#include "../Defines.hpp"

struct SGame;

struct SApplicationConfig {
	short start_x;
	short start_y;
	short start_width;
	short start_height;

	char* name;
};

bool ApplicationCreate(struct SGame* game_instance);
bool ApplicationRun();