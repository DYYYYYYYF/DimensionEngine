#pragma once

#include <Defines.hpp>
#include <GameType.hpp>
#include <Math/MathTypes.hpp>
#include <Core/CPython.hpp>

class Camera;

class GameInstance : public IGame {
public:
	virtual bool Boot(IRenderer* renderer) override;
	virtual void Shutdown() override;
	virtual bool Initialize() override;
	virtual bool Update(float delta_time) override;
	virtual bool Render(struct SRenderPacket* packet, float delta_time) override;
	virtual void OnResize(unsigned int width, unsigned int height) override;

public:
	float delta_time;
	Camera* WorldCamera = nullptr;
	short Width, Height;

	Frustum CameraFrustum;

	// TODO: temp
	Skybox SB;

	std::vector<Mesh> Meshes;
	Mesh* CarMesh = nullptr;
	Mesh* SponzaMesh = nullptr;
	Mesh* DragonMesh = nullptr;
	Mesh* BunnyMesh = nullptr;

	std::vector<Mesh> UIMeshes;
	UIText TestText;
	UIText TestSysText;

	uint32_t HoveredObjectID;
	CPythonModule TestPython;
	// TODO: end temp

};
