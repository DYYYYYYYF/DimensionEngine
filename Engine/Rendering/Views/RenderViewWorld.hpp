#pragma once

#include "Defines.hpp"
#include "Rendering/Interface/IRenderView.hpp"

class UShader;
class ACameraActor;

class RenderViewWorld : public IRenderView {
public:
	RenderViewWorld();
	RenderViewWorld(const RenderViewConfig& config);
	virtual bool OnCreate(const RenderViewConfig& config) override;
	virtual void OnDestroy() override;
	virtual void OnResize(uint32_t width, uint32_t height) override;
	virtual bool RegenerateAttachmentTarget(uint32_t passIndex, RenderTargetAttachment* attachment) override;

public:
	const FString& GetShaderName() const {
		if (UsedShader->Name.IsEmpty()) {
			return nullptr;
		}

		return CustomShaderName.IsEmpty() ? UsedShader->Name : CustomShaderName;
	}

	void SetShader(UShader* shader) { UsedShader = shader; }
	UShader* GetShader() const { return UsedShader; }


private:
	UShader* UsedShader = nullptr;
	float NearClip;
	float FarClip;
	float Fov;
	Matrix4 ProjectionMatrix;
	ACameraActor* WorldCamera = nullptr;
	Vector4 AmbientColor;
};