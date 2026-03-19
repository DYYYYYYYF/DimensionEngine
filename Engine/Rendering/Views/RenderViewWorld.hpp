#pragma once

#include "Defines.hpp"
#include "Rendering/Interface/IRenderView.hpp"

class Shader;
class ACameraActor;

class RenderViewWorld : public IRenderView {
public:
	RenderViewWorld();
	RenderViewWorld(const RenderViewConfig& config);
	virtual bool OnCreate(const RenderViewConfig& config) override;
	virtual void OnDestroy() override;
	virtual void OnResize(uint32_t width, uint32_t height) override;
	virtual bool OnBuildPacket(IRenderviewPacketData* data, struct RenderViewPacket* out_packet) override;
	virtual void OnDestroyPacket(struct RenderViewPacket* packet) override;
	virtual bool OnRender(struct RenderViewPacket* packet, IRendererBackend* back_renderer, size_t frame_number, size_t render_target_index) override;
	virtual bool RegenerateAttachmentTarget(uint32_t passIndex, RenderTargetAttachment* attachment) override;

public:
	const char* GetShaderName() const {
		if (UsedShader->Name.IsEmpty()) {
			return nullptr;
		}

		return CustomShaderName ? CustomShaderName : UsedShader->Name.CStr();
	}

	void SetShader(Shader* shader) { UsedShader = shader; }
	Shader* GetShader() const { return UsedShader; }


private:
	Shader* UsedShader = nullptr;
	float NearClip;
	float FarClip;
	float Fov;
	Matrix4 ProjectionMatrix;
	ACameraActor* WorldCamera = nullptr;
	Vector4 AmbientColor;
};