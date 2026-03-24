#include "RenderViewSkybox.hpp"

#include "Core/EngineLogger.hpp"
#include "Core/DMemory.hpp"
#include "Core/Event.hpp"
#include "Math/DMath.hpp"

#include "Containers/TArray.hpp"
#include "Rendering/Resources/Skybox/Skybox.hpp"

#include "Systems/MaterialSystem.h"
#include "Systems/ShaderSystem.h"
#include "Systems/CameraSystem.h"
#include "Systems/ResourceSystem.h"
#include "Systems/RenderViewSystem.hpp"

#include "Rendering/Renderer.hpp"
#include "Rendering/Interface/IRenderpass.hpp"
#include "Rendering/Interface/IRendererBackend.hpp"

static bool RenderViewSkyboxOnEvent(eEventCode code, void* sender, void* listenerInst, SEventContext context) {
	IRenderView* self = (IRenderView*)listenerInst;
	if (self == nullptr) {
		return false;
	}

	switch (code)
	{
	case eEventCode::Default_Rendertarget_Refresh_Required:
		RenderViewSystem::Get().RegenerateRendertargets(self);
		return false;
    default: break;
	}

	return false;
}

RenderViewSkybox::RenderViewSkybox() {
	
}

RenderViewSkybox::RenderViewSkybox(const RenderViewConfig& config) {
	Type = config.type;
	Name = config.name;
	CustomShaderName = config.custom_shader_name;
	RenderpassCount = config.pass_count;
	Passes.resize(RenderpassCount);
}

bool RenderViewSkybox::OnCreate(const RenderViewConfig& config) {
	FString ShaderName = "Shader.Builtin.Skybox";
	UAsset ConfigResource;
	if (!ResourceSystem::Get().Load(ShaderName, EAssetType::Shader, nullptr, &ConfigResource)) {
		GLOG(Log::eError, "Failed to load builtin skybox shader.");
		return false;
	}

	ShaderConfig* Config = (ShaderConfig*)ConfigResource.Data;
	// NOTE: Assuming the first pass since that's all this view has.
	if (!ShaderSystem::Get().Create(&Passes[0], Config)) {
		GLOG(Log::eError, "Failed to load builtin ksybox shader.");
		return false;
	}
	ResourceSystem::Get().Unload(&ConfigResource);

	// Get either the custom shader override or the defined default.
	UsedShader = ShaderSystem::Get().Get(CustomShaderName.IsEmpty() ? ShaderName : CustomShaderName);
	ProjectionLocation = UsedShader->GetUniformIndex("projection");
	ViewLocation = UsedShader->GetUniformIndex("view");
	CubeMapLocation = UsedShader->GetUniformIndex("cube_texture");
	
	// TODO: Set from configurable.
	NearClip = 0.1f;
	FarClip = 1000.0f;
	Fov = Deg2Rad(45.0f);

	// Default
	ProjectionMatrix = Matrix4::Perspective(Fov, (float)config.width / config.height, NearClip, FarClip);
	WorldCamera = CameraSystem::Get().GetDefault();

	if (!EngineEvent::Register(eEventCode::Default_Rendertarget_Refresh_Required, this, RenderViewSkyboxOnEvent)) {
		GLOG(Log::eError, "Unable to listen for refresh required event, creation failed.");
		return false;
	}

	GLOG(Log::eInfo, "Renderview skybox created.");
	return true;
}

void RenderViewSkybox::OnDestroy() {
	EngineEvent::Unregister(eEventCode::Default_Rendertarget_Refresh_Required, this, RenderViewSkyboxOnEvent);
}

void RenderViewSkybox::OnResize(uint32_t width, uint32_t height) {
	// Check if different.
	if (width == Width && height == Height) {
		return;
	}

	Width = (uint16_t)width;
	Height = (uint16_t)height;
	ProjectionMatrix = Matrix4::Perspective(Fov, (float)Width / (float)Height, NearClip, FarClip);

	for (uint32_t i = 0; i < RenderpassCount; ++i) {
		Passes[i].SetRenderArea(Vector4(0, 0, (float)Width, (float)Height));
	}
}

bool RenderViewSkybox::OnBuildPacket(IRenderviewPacketData* data, struct RenderViewPacket* out_packet) {
	if (data == nullptr || out_packet == nullptr) {
		GLOG(Log::eWarn, "RenderViewSkybox::OnBuildPacke() Requires valid pointer to packet and data.");
		return false;
	}

	SkyboxPacketData* SkyboxData = (SkyboxPacketData*)data;
	out_packet->view = this;

	// Set matrix, etc.
	out_packet->projection_matrix = ProjectionMatrix;
	out_packet->view_matrix = WorldCamera->GetViewMatrix();
	out_packet->view_position = WorldCamera->GetPosition();

	// Just set the extended data to the skybox data.
	out_packet->extended_data = NewObject<SkyboxPacketData>(*SkyboxData);

	return true;
}

void RenderViewSkybox::OnDestroyPacket(struct RenderViewPacket* packet) {
	if (packet->extended_data) {
		DeleteObject(packet->extended_data);
		packet->extended_data = nullptr;
	}

	// No much to do here, just zero mem.
	Memory::Zero(packet, sizeof(RenderViewPacket));
}

bool RenderViewSkybox::RegenerateAttachmentTarget(uint32_t passIndex, RenderTargetAttachment* attachment) {
	return false;
}

bool RenderViewSkybox::OnRender(struct RenderViewPacket* packet, RHI* back_renderer, size_t frame_number, size_t render_target_index) {
	SkyboxPacketData* SkyboxData = (SkyboxPacketData*)packet->extended_data;

	for (uint32_t p = 0; p < RenderpassCount; ++p) {
		IRenderpass* Pass = (IRenderpass*)&Passes[p];
		Pass->Begin(&Pass->Targets[render_target_index]);

		if (!UsedShader->Use()) {
			GLOG(Log::eError, "RenderViewSkybox::OnRender() Failed to use material shader. Render frame failed.");
			return false;
		}

		// Get the view matrix, but zero out the position so the skybox stays put on screen.
		Matrix4 ViewMatrix = WorldCamera->GetViewMatrix();
		ViewMatrix.data[12] = 0.0f;
		ViewMatrix.data[13] = 0.0f;
		ViewMatrix.data[14] = 0.0f;

		// Apply globals
		// TODO: This is terrible
		UsedShader->BindGlobal();
		if (!UsedShader->SetUniformByIndex(ProjectionLocation, &packet->projection_matrix)) {
			GLOG(Log::eError, "RenderViewSkybox::OnRender() Failed to apply skybox projection uniform.");
			return false;
		}

		if (!UsedShader->SetUniformByIndex(ViewLocation, &ViewMatrix)) {
			GLOG(Log::eError, "RenderViewSkybox::OnRender() Failed to apply skybox view uniform.");
			return false;
		}

		UsedShader->ApplyGlobal();

		// Instance.
		UsedShader->BindInstance(SkyboxData->sb->InstanceID);
		if (!UsedShader->SetUniformByIndex(CubeMapLocation, &SkyboxData->sb->CubeMap)) {
			GLOG(Log::eError, "RenderViewSkybox::OnRender() Failed to apply cube map uniform.");
			return false;
		}

		bool NeedsUpdate = SkyboxData->sb->RenderFrameNumber != frame_number;
		UsedShader->ApplyInstance(NeedsUpdate);

		// Sync the frame num.
		SkyboxData->sb->RenderFrameNumber = frame_number;

		// Draw
		GeometryRenderData RenderData;
		RenderData.geometry = SkyboxData->sb->g;
		back_renderer->DrawGeometry(&RenderData);

		Pass->End();
	}

	return true;
}
