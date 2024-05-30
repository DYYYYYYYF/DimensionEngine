#include "Game.hpp"

#include <Core/EngineLogger.hpp>
#include <Core/Input.hpp>
#include <Core/Event.hpp>
#include <Systems/CameraSystem.h>


bool GameInitialize(SGame* game_instance) {
	UL_DEBUG("GameInitialize() called.");

	SGameState* State = (SGameState*)game_instance->state;

	State->WorldCamera = CameraSystem::GetDefault();
	State->WorldCamera->SetPosition(Vec3(0.0f, 0.0f, -40.0f));

	return true;
}

bool GameUpdate(SGame* game_instance, float delta_time) {
	SGameState* State = (SGameState*)game_instance->state;

	if (Core::InputIsKeyDown(eKeys_Left)) {
		State->WorldCamera->RotateYaw(-1.0f * delta_time);
	}
	if (Core::InputIsKeyDown(eKeys_Right)) {
		State->WorldCamera->RotateYaw(1.0f * delta_time);
	}
	if (Core::InputIsKeyDown(Keys::eKeys_Up)) {
		State->WorldCamera->RotatePitch(-1.0f * delta_time);
	}
	if (Core::InputIsKeyDown(Keys::eKeys_Down)) {
		State->WorldCamera->RotatePitch(1.0f * delta_time);
	}

	float TempMoveSpeed = 50.0f;
	if (Core::InputIsKeyDown(Keys::eKeys_Shift)) {
		TempMoveSpeed *= 2.0f;
	}

	if (Core::InputIsKeyDown(Keys::eKeys_W)) {
		State->WorldCamera->MoveForward(TempMoveSpeed * delta_time);
	}
	if (Core::InputIsKeyDown(Keys::eKeys_S)) {
		State->WorldCamera->MoveBackward(TempMoveSpeed * delta_time);
	}
	if (Core::InputIsKeyDown(Keys::eKeys_A)) {
		State->WorldCamera->MoveLeft(TempMoveSpeed * delta_time);
	}
	if (Core::InputIsKeyDown(Keys::eKeys_D)) {
		State->WorldCamera->MoveRight(TempMoveSpeed * delta_time);
	}
	if (Core::InputIsKeyDown(Keys::eKeys_Q)) {
		State->WorldCamera->MoveDown(TempMoveSpeed * delta_time);
	}
	if (Core::InputIsKeyDown(Keys::eKeys_E)) {
		State->WorldCamera->MoveUp(TempMoveSpeed * delta_time);
	}

	if (Core::InputIsKeyDown(Keys::eKeys_R)) {
		State->WorldCamera->Reset();
	}

	int px, py, cx, cy;
	Core::InputGetMousePosition(cx, cy);
	Core::InputGetPreviousMousePosition(px, py);
	float MouseMoveSpeed = 0.005f;
	if (Core::InputeIsButtonDown(eButton_Right)) {
		if (cx != px) {
			State->WorldCamera->RotateYaw((cx - px) * MouseMoveSpeed);
		}
		if (cy != py) {
			State->WorldCamera->RotatePitch((cy - py) * MouseMoveSpeed);
		}
	}

	return true;
}

bool GameRender(SGame* game_instance, float delta_time) {

	return true;
}

void GameOnResize(SGame* game_instance, unsigned int width, unsigned int height) {

}
