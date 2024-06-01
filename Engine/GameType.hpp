#pragma once

#include "Core/Application.hpp"

struct SGame {
	SApplicationConfig app_config;

	bool (*initialize)(struct SGame* game_instance);
	bool (*update)(struct SGame* game_instance, float delta_time);
	bool (*render)(struct SGame* game_instance, float delta_time);

	void (*on_resize)(struct SGame* game_instance, unsigned int width, unsigned int height);

	void* state = nullptr;
};
