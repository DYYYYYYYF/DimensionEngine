#include "Skybox.hpp"

#include "Core/EngineLogger.hpp"
#include "Rendering/Renderer.hpp"
#include "Systems/ShaderSystem.h"
#include "Systems/TextureSystem.h"
#include "Systems/GeometrySystem.h"

bool Skybox::Create(const FString& cubeName) {
	Renderer = IRenderer::GetRenderer();

	SGeometryConfig SkyboxCubeConfig = GeometrySystem::Get().GenerateCubeConfig(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, cubeName, FString());

	// Clear out the material name.
	g = GeometrySystem::Get().AcquireFromConfig(SkyboxCubeConfig, true);
	g->IncreaseReferenceCount();
	RenderFrameNumber = INVALID_ID_U64;

	Material* Mat = MaterialSystem::Get().Acquire("Material.Builtin.Skybox");
	if (!Mat) {
		return false;
	}

	g->Material = Mat;

	return true;
}

void Skybox::Destroy() {
	TextureSystem::Get().Release("skybox");

	if (g) {
		g->DecreaseReferenceCount();
		g = nullptr;
	}
}
