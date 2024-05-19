#include "Game.hpp"
#include "Core/EngineLogger.hpp"
#include "Core/Input.hpp"

#include "Renderer/RendererFrontend.hpp"


void RecalculateViewMatrix(SGameState* state) {
	if (state->camera_view_dirty) {
		Matrix4 Rotation = Matrix4::EulerXYZ(state->camera_euler.x, state->camera_euler.y, state->camera_euler.z);
		Matrix4 Translation = Matrix4::Identity();
		Translation.SetTranslation(state->camera_position);

		state->camera_view_dirty = false;
	}
}

void CameraYaw(SGameState* state, float amount) {
	state->camera_euler.y += amount;
	state->camera_view_dirty = true;
}

void CameraPitch(SGameState* state, float amount) {
	state->camera_euler.x += amount;

	float Limit = Deg2Rad(89.5f);
	state->camera_euler.x = CLAMP(state->camera_euler.x, -Limit, Limit);

	state->camera_view_dirty = true;
}

bool GameInitialize(SGame* game_instance) {
	UL_DEBUG("GameInitialize() called.");

	SGameState* State = (SGameState*)game_instance->state;

	State->camera_position = Vec3{ 0, 0, -30.f };
	State->camera_euler = Vec3();

	State->view = Matrix4::Identity();
	State->view.SetTranslation(State->camera_position);

	State->camera_view_dirty = true;

	return true;
}

bool GameUpdate(SGame* game_instance, float delta_time) {

	if (Core::InputIsKeyDown(eKeys_A) || Core::InputIsKeyDown(eKeys_Left)) {
		CameraYaw((SGameState*)game_instance->state, 1.0f * delta_time);
	}
	if (Core::InputIsKeyDown(eKeys_D) || Core::InputIsKeyDown(eKeys_Right)) {
		CameraYaw((SGameState*)game_instance->state, -1.0f * delta_time);
	}

	RecalculateViewMatrix((SGameState*)game_instance->state);
	((SGameState*)game_instance->state)->view;

	return true;
}

bool GameRender(SGame* game_instance, float delta_time) {

	return true;
}

void GameOnResize(SGame* game_instance, unsigned int width, unsigned int height) {

}
