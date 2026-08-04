#include "Skybox.hpp"

#include "Core/EngineLogger.hpp"
#include "Rendering/Renderer.hpp"
#include "Systems/ShaderSystem.h"
#include "Systems/TextureSystem.h"
#include "Systems/GeometrySystem.h"

bool USkybox::Create(const FString& cubeName) {
	Renderer = IRenderer::GetRenderer();

	FGeometryConfig SkyboxCubeConfig = GeometrySystem::Get().GenerateCubeConfig(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, cubeName, FString());

	// Clear out the material name.
	geo = GeometrySystem::Get().AcquireFromConfig(SkyboxCubeConfig, true);
	RenderFrameNumber = INVALID_ID_U64;

	UMaterial* Mat = MaterialSystem::Get().Acquire("Material.Builtin.Skybox");
	if (!Mat) {
		return false;
	}

	geo->SetMaterial(Mat);

	return true;
}

void USkybox::Destroy() {
	if (geo) {
		DeleteObject(geo);
		geo = nullptr;
	}

	TextureSystem::Get().Release("skybox");
}
