#pragma once

#include <Defines.hpp>
#include <GameType.hpp>
#include <Math/MathTypes.hpp>

class Camera;

struct SGameState {
	float delta_time;
	Camera* WorldCamera = nullptr;
	short Width, Height;

	// TODO: temp
	Skybox SB;

	std::vector<Mesh> Meshes;
	Mesh* CarMesh;
	Mesh* SponzaMesh;
	bool ModelsLoaded;

	std::vector<Mesh> UIMeshes;
	UIText TestText;
	UIText TestSysText;

	uint32_t HoveredObjectID;
	// TODO: end temp

};

bool GameBoot(SGame* gameInstance, IRenderer* renderer);
bool GameInitialize(SGame* game_instance);
bool GameUpdate(SGame* game_instance, float delta_time);
bool GameRender(SGame* game_instance, struct SRenderPacket* packet, float delta_time);
void GameOnResize(SGame* game_instance, unsigned int width, unsigned int height);
void GameShutdown(SGame* gameInstance);