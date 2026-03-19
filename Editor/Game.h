#pragma once

#include "UI/Console/GameConsole.h"
#include <Defines.hpp>
#include <IGame.hpp>
#include <Math/MathTypes.hpp>
#include <Core/CPython.hpp>
#include <Core/Keymap.hpp>
#include <Containers/TArray.hpp>
#include "Framework/Classes/StaticMeshActor.h"
#include "Framework/Classes/TextActor.h"

#define EDITOR_CONFIG_PATH FString::Format("%s%s", ROOT_PATH, "/Editor/Config.json")

class Skybox;
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
	Skybox* SB;
	Keymap* ConsoleKeymap;
	DebugConsoleActor* GameConsole = nullptr;

	TArray<AStaticMeshActor*> Meshes;
	TArray<AStaticMeshActor*> UIMeshes;
	ATextActor* TestText = nullptr;
	ATextActor* TestSysText = nullptr;

	uint32_t HoveredObjectID = INVALID_ID;
	CPythonModule TestPython;
	// TODO: end temp

};
