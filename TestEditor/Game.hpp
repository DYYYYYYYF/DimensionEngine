#pragma once

#include <../engine/Defines.hpp>
#include <../engine/GameType.hpp>

struct SGameState {
	float delta_time;
};

bool GameInitialize(SGame* game_instance);
bool GameUpdate(SGame* game_instance, float delta_time);
bool GameRender(SGame* game_instance, float delta_time);
void GameOnResize(SGame* game_instance, unsigned int width, unsigned int height);