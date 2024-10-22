#include "RenderViewSkybox.hpp"

#include "Core/EngineLogger.hpp"
#include "Core/DMemory.hpp"
#include "Core/Event.hpp"
#include "Math/DMath.hpp"
#include "Math/Transform.hpp"
#include "Containers/TArray.hpp"
#include "Containers/TString.hpp"
#include "Resources/Skybox.hpp"

#include "Systems/MaterialSystem.h"
#include "Systems/ShaderSystem.h"
#include "Systems/CameraSystem.h"
#include "Systems/ResourceSystem.h"
#include "Systems/RenderViewSystem.hpp"

#include "Renderer/RendererFrontend.hpp"
#include "Renderer/Interface/IRenderpass.hpp"
#include "Renderer/Interface/IRendererBackend.hpp"

static bool RenderViewSkyboxOnEvent(eEventCode code, void* sender, void* listenerInst, SEventContext context) {
	IRenderView* self = (IRenderView*)listenerInst;
	if (self == nullptr) {
		return false;
	}

	switch (code)
	{
	case eEventCode::Default_Rendertarget_Refresh_Required:
		RenderViewSystem::RegenerateRendertargets(self);
		return false;
    default: break;
	}

	return false;
}

RenderViewSkybox::RenderViewSkybox() {
	
}

RenderViewSkybox::RenderViewSkybox(const RenderViewConfig& config) {
	Type = config.type;
	Name = StringCopy(config.name);
	CustomShaderName = config.custom_shader_name;
	RenderpassCount = config.pass_count;
	Passes.resize(RenderpassCount);
}

bool RenderViewSkybox::OnCreate(const RenderViewConfig& config) {
	const char* ShaderName = "Shader.Builtin.Skybox";
	Resource ConfigResource;
	if (!ResourceSystem::Load(ShaderName, ResourceType::eResource_Type_Shader, nullptr, &ConfigResource)) {
		LOG_ERROR("Failed to load builtin skybox shader.");
		return false;
	}

	ShaderConfig* Config = (ShaderConfig*)ConfigResource.Data;
	// NOTE: Assuming the first pass since that's all this view has.
	if (!ShaderSystem::Create(&Passes[0], Config)) {
		LOG_ERROR("Failed to load builtin ksybox shader.");
		return false;
	}
	ResourceSystem::Unload(&ConfigResource);

	// Get either the custom shader override or the defined default.
	UsedShader = ShaderSystem::Get(CustomShaderName ? CustomShaderName : ShaderName);
	ProjectionLocation = ShaderSystem::GetUniformIndex(UsedShader, "projection");
	ViewLocation = ShaderSystem::GetUniformIndex(UsedShader, "view");
	CubeMapLocation = ShaderSystem::GetUniformIndex(UsedShader, "cube_texture");
	
	// TODO: Set from configurable.
	NearClip = 0.1f;
	FarClip = 1000.0f;
	Fov = Deg2Rad(45.0f);

	// Default
	ProjectionMatrix = Matrix4::Perspective(Fov, 1280.0f / 720.0f, NearClip, FarClip);
	WorldCamera = CameraSystem::GetDefault();

	if (!EngineEvent::Register(eEventCode::Default_Rendertarget_Refresh_Required, this, RenderViewSkyboxOnEvent)) {
		LOG_ERROR("Unable to listen for refresh required event, creation failed.");
		return false;
	}

	LOG_INFO("Renderview skybox created.");
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

	Width = width;
	Height = height;
	ProjectionMatrix = Matrix4::Perspective(Fov, (float)Width / (float)Height, NearClip, FarClip);

	for (uint32_t i = 0; i < RenderpassCount; ++i) {
		Passes[i].SetRenderArea(Vec4(0, 0, (float)Width, (float)Height));
	}
}

bool RenderViewSkybox::OnBuildPacket(void* data, struct RenderViewPacket* out_packet) {
	if (data == nullptr || out_packet == nullptr) {
		LOG_WARN("RenderViewSkybox::OnBuildPacke() Requires valid pointer to packet and data.");
		return false;
	}

	SkyboxPacketData* SkyboxData = (SkyboxPacketData*)data;
	out_packet->view = this;

	// Set matrix, etc.
	out_packet->projection_matrix = ProjectionMatrix;
	out_packet->view_matrix = WorldCamera->GetViewMatrix();
	out_packet->view_position = WorldCamera->GetPosition();

	// Just set the extended data to the skybox data.
	out_packet->extended_data = Memory::Allocate(sizeof(SkyboxPacketData), MemoryType::eMemory_Type_Renderer);
	// Copy over the packet data.
	Memory::Copy(out_packet->extended_data, SkyboxData, sizeof(SkyboxPacketData));

	return true;
}

void RenderViewSkybox::OnDestroyPacket(struct RenderViewPacket* packet) {
	if (packet->extended_data) {
		Memory::Free(packet->extended_data, sizeof(SkyboxPacketData), eMemory_Type_Renderer);
		packet->extended_data = nullptr;
	}

	// No much to do here, just zero mem.
	Memory::Zero(packet, sizeof(RenderViewPacket));
}

bool RenderViewSkybox::RegenerateAttachmentTarget(uint32_t passIndex, RenderTargetAttachment* attachment) {
	return false;
}

bool RenderViewSkybox::OnRender(struct RenderViewPacket* packet, IRendererBackend* back_renderer, size_t frame_number, size_t render_target_index) {
	uint32_t SID = UsedShader->ID;
	SkyboxPacketData* SkyboxData = (SkyboxPacketData*)packet->extended_data;
	for (uint32_t p = 0; p < RenderpassCount; ++p) {
		IRenderpass* Pass = (IRenderpass*)&Passes[p];
		Pass->Begin(&Pass->Targets[render_target_index]);

		if (!ShaderSystem::UseByID(SID)) {
			LOG_ERROR("RenderViewSkybox::OnRender() Failed to use material shader. Render frame failed.");
			return false;
		}

		// Get the view matrix, but zero out the position so the skybox stays put on screen.
		Matrix4 ViewMatrix = WorldCamera->GetViewMatrix();
		ViewMatrix.data[12] = 0.0f;
		ViewMatrix.data[13] = 0.0f;
		ViewMatrix.data[14] = 0.0f;

		// Apply globals
		// TODO: This is terrible
		back_renderer->BindGlobalsShader(ShaderSystem::GetByID(SID));
		if (!ShaderSystem::SetUniformByIndex(ProjectionLocation, &packet->projection_matrix)) {
			LOG_ERROR("RenderViewSkybox::OnRender() Failed to apply skybox projection uniform.");
			return false;
		}

		if (!ShaderSystem::SetUniformByIndex(ViewLocation, &ViewMatrix)) {
			LOG_ERROR("RenderViewSkybox::OnRender() Failed to apply skybox view uniform.");
			return false;
		}

		ShaderSystem::ApplyGlobal();

		// Instance.
		ShaderSystem::BindInstance(SkyboxData->sb->InstanceID);
		if (!ShaderSystem::SetUniformByIndex(CubeMapLocation, &SkyboxData->sb->CubeMap)) {
			LOG_ERROR("RenderViewSkybox::OnRender() Failed to apply cube map uniform.");
			return false;
		}

		bool NeedsUpdate = SkyboxData->sb->RenderFrameNumber != frame_number;
		ShaderSystem::ApplyInstance(NeedsUpdate);

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
