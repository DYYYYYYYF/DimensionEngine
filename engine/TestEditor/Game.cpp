#include "Game.hpp"
#include <core/EngineLogger.hpp>

bool GameInitialize(SGame* game_instance) {
	UL_DEBUG("GameInitialize() called.");
	return true;
}

bool GameUpdate(SGame* game_instance, float delta_time) {

	return true;
}

bool GameRender(SGame* game_instance, float delta_time) {

	return true;
}

void GameOnResize(SGame* game_instance, unsigned int width, unsigned int height) {

}
