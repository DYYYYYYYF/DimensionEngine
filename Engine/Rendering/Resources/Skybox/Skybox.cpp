#include "Skybox.hpp"

#include "Core/EngineLogger.hpp"
#include "Rendering/Renderer.hpp"
#include "Systems/ShaderSystem.h"
#include "Systems/TextureSystem.h"
#include "Systems/GeometrySystem.h"

bool Skybox::Create(const FString& cubeName) {
	Renderer = IRenderer::GetRenderer();

	CubeMap.filter_magnify = TextureFilter::eTexture_Filter_Mode_Linear;
	CubeMap.filter_minify = TextureFilter::eTexture_Filter_Mode_Linear;
	CubeMap.repeat_u = TextureRepeat::eTexture_Repeat_Clamp_To_Edge;
	CubeMap.repeat_v = TextureRepeat::eTexture_Repeat_Clamp_To_Edge;
	CubeMap.repeat_w = TextureRepeat::eTexture_Repeat_Clamp_To_Edge;
	CubeMap.usage = TextureUsage::eTexture_Usage_Map_Cubemap;
	if (!Renderer->AcquireTextureMap(&CubeMap)) {
		GLOG(Log::eFatal, "Unable to acquire resources for cube map texture.");
		return false;
	}

	CubeMap.texture = TextureSystem::AcquireCube("skybox", true);
	SGeometryConfig SkyboxCubeConfig = GeometrySystem::Get().GenerateCubeConfig(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, cubeName, FString());

	// Clear out the material name.
	g = GeometrySystem::Get().AcquireFromConfig(SkyboxCubeConfig, true);
	RenderFrameNumber = INVALID_ID_U64;
	Shader* SkyboxShader = ShaderSystem::Get("Shader.Builtin.Skybox");
	if (SkyboxShader == nullptr) {
		GLOG(Log::eWarn, "Skybox shader is not loaded. Maybe there is no skybox render pass be created.");
		return true;
	}

	std::vector<TextureMap*> Maps = { &CubeMap };

	InstanceID = Renderer->AcquireInstanceResource(SkyboxShader, Maps);
	if (InstanceID == INVALID_ID) {
		GLOG(Log::eFatal, "Unable to acquire shader resources for skybox texture.");
		return false;
	}

	return true;
}

void Skybox::Destroy() {
	Renderer->ReleaseTextureMap(&CubeMap);
}
