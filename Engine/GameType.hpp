#pragma once

#include "Core/Application.hpp"

static IRenderer* Renderer = nullptr;

struct SGame {
	SApplicationConfig app_config;

	bool (*boot)(struct SGame* game_instance, IRenderer* renderer);
	void (*shutdown)(struct SGame* game_instance);

	bool (*initialize)(struct SGame* game_instance);
	bool (*update)(struct SGame* game_instance, float delta_time);
	bool (*render)(struct SGame* game_instance, struct SRenderPacket* packet, float delta_time);

	void (*on_resize)(struct SGame* game_instance, unsigned int width, unsigned int height);

	void* state = nullptr;
};
