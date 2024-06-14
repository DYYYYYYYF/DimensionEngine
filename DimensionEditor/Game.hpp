#pragma once

#include <Defines.hpp>
#include <GameType.hpp>
#include <Math/MathTypes.hpp>

class Camera;

struct SGameState {
	float delta_time;
	Camera* WorldCamera;
};

bool GameInitialize(SGame* game_instance);
bool GameUpdate(SGame* game_instance, float delta_time);
bool GameRender(SGame* game_instance, float delta_time);
void GameOnResize(SGame* game_instance, unsigned int width, unsigned int height);