#pragma once

#include "../Defines.hpp"

struct SGame;
struct SEventContext;

struct SApplicationConfig {
	short start_x;
	short start_y;
	short start_width;
	short start_height;

	char* name;
};

bool ApplicationCreate(struct SGame* game_instance);
bool ApplicationRun();

// Event handlers
bool ApplicationOnEvent(unsigned short code, void* sender, void* listener_instance, SEventContext context);
bool ApplicationOnKey(unsigned short code, void* sender, void* listener_instance, SEventContext context);
