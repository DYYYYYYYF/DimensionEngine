#pragma once

#include "Defines.hpp"
#include "Rendering/Resources/Texture/Texture.hpp"
#include "Rendering/Interface/IRenderView.hpp"

class Shader;
class ACameraActor;

// G-Buffer纹理
struct GBufferSet {
	UTexture* AlbedoTexture = nullptr;
	UTexture* NormalTexture = nullptr;
	UTexture* PositionTexture = nullptr;
	UTexture* DepthTexture = nullptr;

	TextureMap AlbedoTextureMap;
	TextureMap NormalTextureMap;
	TextureMap PositionTextureMap;
	TextureMap DepthTextureMap;
};

class RenderViewWorldDeferred : public IRenderView {
public:
	RenderViewWorldDeferred();
	RenderViewWorldDeferred(const RenderViewConfig& config);
	virtual bool OnCreate(const RenderViewConfig& config) override;
	virtual void OnDestroy() override;
	virtual void OnResize(uint32_t width, uint32_t height) override;
	virtual bool OnBuildPacket(IRenderviewPacketData* data, struct RenderViewPacket* out_packet) override;
	virtual void OnDestroyPacket(struct RenderViewPacket* packet) override;
	virtual bool OnRender(struct RenderViewPacket* packet, IRendererBackend* back_renderer, size_t frame_number, size_t render_target_index) override;
	virtual bool RegenerateAttachmentTarget(uint32_t passIndex, RenderTargetAttachment* attachment) override;

public:
	const char* GetShaderName() const {
		if (GBufferShader->Name.IsEmpty()) {
			return nullptr;
		}
		return GBufferShader->Name.CStr();
	}

	void SetGBufferShader(Shader* shader) { GBufferShader = shader; }
	void SetLightingShader(Shader* shader) { LightingShader = shader; }
	Shader* GetGBufferShader() const { return GBufferShader; }
	Shader* GetLightingShader() const { return LightingShader; }

private:
	GBufferSet* GetCurrentGBufferSet(size_t render_target_index) {
		return &GBuffers[render_target_index % MAX_RENDER_TARGETS];
	}

private:
	IRenderer* Renderer;

	// G-Buffer渲染着色器
	Shader* GBufferShader = nullptr;
	// 光照计算着色器
	Shader* LightingShader = nullptr;

	float NearClip;
	float FarClip;
	float Fov;
	Matrix4 ProjectionMatrix;
	ACameraActor* WorldCamera = nullptr;
	Vector4 AmbientColor;

	static const uint32_t MAX_RENDER_TARGETS = 3;  // 双缓冲
	GBufferSet GBuffers[MAX_RENDER_TARGETS];        // 两套完整的G-Buffer

	uint32_t InstanceID = INVALID_ID;

	// 全屏四边形用于光照计算
	Geometry* FullscreenQuad = nullptr;

	// 着色器uniform位置
	struct GBufferUniforms {
		unsigned short ModelLocation;
		unsigned short ViewLocation;
		unsigned short ProjectionLocation;
		// 材质属性
		unsigned short DiffuseColorLocation;
		unsigned short MetallicLocation;
		unsigned short RoughnessLocation;
		unsigned short AmbientOcclusionLocation;
		unsigned short NormalIntensityLocation;
		// 纹理采样器
		unsigned short DiffuseMapLocation;
		unsigned short NormalMapLocation;
		unsigned short MetallicRoughnessMapLocation;
	} GBufferUniforms;

	struct LightingUniforms {
		unsigned short AlbedoMapLocation;
		unsigned short NormalMapLocation;
		unsigned short PositionMapLocation;
		unsigned short ViewPositionLocation;
		unsigned short AmbientColorLocation;
		unsigned short ModeLocation;
		unsigned short TimeLocation;
		unsigned short LightIntensityLocation;
		unsigned short DebugModeLocation;
	} LightingUniforms;

	// 辅助方法
	bool CreateGBufferTextures(uint32_t width, uint32_t height);
	void DestroyGBufferTextures();
	bool CreateFullscreenQuad();
	void DestroyFullscreenQuad();
};