#pragma once

#include "Defines.hpp"

struct SGame;
struct SEventContext;

struct SApplicationConfig {
	short start_x;
	short start_y;
	short start_width;
	short start_height;

	char* name;
};

DAPI bool ApplicationCreate(struct SGame* game_instance);
DAPI bool ApplicationRun();

void GetFramebufferSize(unsigned int* width, unsigned int* height);

// Event handlers
bool ApplicationOnEvent(unsigned short code, void* sender, void* listener_instance, SEventContext context);
bool ApplicationOnKey(unsigned short code, void* sender, void* listener_instance, SEventContext context);
