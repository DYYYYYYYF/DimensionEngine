#pragma once

#include <Defines.hpp>
#include <GameType.hpp>
#include "Math/MathTypes.hpp"

struct SGameState {
	float delta_time;

	Matrix4 view;
	Vec3 camera_position;
	Vec3 camera_euler;
	bool camera_view_dirty;
};

bool GameInitialize(SGame* game_instance);
bool GameUpdate(SGame* game_instance, float delta_time);
bool GameRender(SGame* game_instance, float delta_time);
void GameOnResize(SGame* game_instance, unsigned int width, unsigned int height);