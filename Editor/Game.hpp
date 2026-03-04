#pragma once

#include "GameConsole.hpp"
#include <Defines.hpp>
#include <IGame.hpp>
#include <Math/MathTypes.hpp>
#include <Core/CPython.hpp>
#include <Core/Keymap.hpp>
#include <Containers/TArray.hpp>
#include "Framework/Classes/StaticMeshActor.hpp"
#include "Framework/Classes/TextActor.h"

#define EDITOR_CONFIG_PATH std::string(ROOT_PATH) + "/Editor/Config.json"

class ACameraActor;

class GameInstance : public IGame {
public:
	GameInstance() : WorldCamera(nullptr), ConsoleKeymap(nullptr){}
	virtual ~GameInstance() {};

public:
	virtual bool Boot(IRenderer* renderer) override;
	virtual void Shutdown() override;
	virtual bool Initialize() override;
	virtual bool Update(float delta_time) override;
	virtual bool Render(struct SRenderPacket* packet, float delta_time) override;
	virtual void OnResize(unsigned int width, unsigned int height) override;

private:
	bool ConfigureRenderviews();

public:
	ACameraActor* WorldCamera;
	Frustum CameraFrustum;

	// TODO: temp
	Skybox SB;
	Keymap* ConsoleKeymap;
	DebugConsoleActor* GameConsole = nullptr;

	TArray<AStaticMeshActor*> Meshes;
	TArray<ATextActor*> UIMeshes;
	UIText TestText;
	UIText TestSysText;

	uint32_t HoveredObjectID = INVALID_ID;
	CPythonModule TestPython;
	// TODO: end temp

};
