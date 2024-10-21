#pragma once

#include "Defines.hpp"
#include "Resources/Skybox.hpp"
#include "Systems/FontSystem.hpp"
#include "Renderer/Interface/IRenderView.hpp"

class IGame;
struct SEventContext;

struct SApplicationConfig {
	short start_x;
	short start_y;
	short start_width;
	short start_height;

	const char* name = nullptr;

	FontSystemConfig FontConfig;
	std::vector<RenderViewConfig> Renderviews;
};

DAPI bool ApplicationCreate(IGame* game_instance);
DAPI bool ApplicationRun();

void GetFramebufferSize(unsigned int* width, unsigned int* height);

// Event handlers
bool ApplicationOnEvent(unsigned short code, void* sender, void* listener_instance, SEventContext context);
bool ApplicationOnResized(unsigned short code, void* sender, void* listener_instance, SEventContext context);
