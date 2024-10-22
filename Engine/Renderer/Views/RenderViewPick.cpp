#include "RenderViewPick.hpp"

#include "Core/EngineLogger.hpp"
#include "Core/DMemory.hpp"
#include "Core/Event.hpp"
#include "Core/UID.hpp"
#include "Math/DMath.hpp"
#include "Math/Transform.hpp"
#include "Containers/TArray.hpp"
#include "Containers/TString.hpp"
#include "Systems/MaterialSystem.h"
#include "Systems/ShaderSystem.h"
#include "Systems/ResourceSystem.h"
#include "Systems/CameraSystem.h"
#include "Systems/RenderViewSystem.hpp"
#include "Renderer/RendererFrontend.hpp"
#include "Renderer/Interface/IRenderpass.hpp"
#include "Renderer/Interface/IRendererBackend.hpp"
#include "Resources/UIText.hpp"

static bool RenderViewPickOnEvent(eEventCode code, void* sender, void* listenerInst, SEventContext context) {
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

static bool OnMouseMoved(eEventCode code, void* sender, void* listenerInst, SEventContext context) {
	if (code == eEventCode::Mouse_Moved) {
		RenderViewPick* self = (RenderViewPick*)listenerInst;
		
		// Update position and regenerate the projection matrix.
		short x = context.data.i16[0];
		short y = context.data.i16[1];
		self->SetMouseX(x);
		self->SetMouseY(y);

		return true;
	}

	return false;
}

void RenderViewPick::AcquireShaderInstance() {
	// UI Shader.
	uint32_t Instance = Renderer->AcquireInstanceResource(UIShaderInfo.UsedShader, std::vector<TextureMap*>());
	if (Instance == INVALID_ID) {
		LOG_ERROR("Failed to acquire shader resource.");
		return;
	}

	// World Shader.
	Instance = Renderer->AcquireInstanceResource(WorlShaderInfo.UsedShader, std::vector<TextureMap*>());
	if (Instance == INVALID_ID) {
		LOG_ERROR("Failed to acquire shader resource.");
		return;
	}

	InstanceCount++;
	InstanceUpdated.push_back(false);
}

void RenderViewPick::ReleaseShaderInstance() {
	for (int i = 0; i < InstanceCount; ++i) {
		// UI Shader
		if (!Renderer->ReleaseInstanceResource(UIShaderInfo.UsedShader, i)) {
			LOG_ERROR("Failed to release shader resource.");
		}

		// UI Shader
		if (!Renderer->ReleaseInstanceResource(WorlShaderInfo.UsedShader, i)) {
			LOG_ERROR("Failed to release shader resource.");
		}
	}

	InstanceUpdated.clear();
}

RenderViewPick::RenderViewPick(const RenderViewConfig& config, IRenderer* renderer) {
	Type = config.type;
	Name = StringCopy(config.name);
	CustomShaderName = config.custom_shader_name;
	RenderpassCount = config.pass_count;
	Passes.resize(RenderpassCount);
	Renderer = renderer;
}

bool RenderViewPick::OnCreate(const RenderViewConfig& config) {
	// NOTE: In this heavily-customized view, the exact number of passes is known, so
	// these index assumptions are fine.
	WorlShaderInfo.Pass = &Passes[0];
	UIShaderInfo.Pass = &Passes[1];

	// Builtin UI Pick shader.
	const char* UIShaderName = "Shader.Builtin.UIPick";
	Resource ConfigResource;
	if (!ResourceSystem::Load(UIShaderName, ResourceType::eResource_Type_Shader, nullptr, &ConfigResource)) {
		LOG_ERROR("Failed to load builtin UI Pick shader.");
		return false;
	}
	ShaderConfig* Config = (ShaderConfig*)ConfigResource.Data;
	if (!ShaderSystem::Create(UIShaderInfo.Pass, Config)) {
		LOG_ERROR("Failed to load builtin UI Pick shader.");
		return false;
	}
	ResourceSystem::Unload(&ConfigResource);
	UIShaderInfo.UsedShader = ShaderSystem::Get(UIShaderName);

	// Extract uniform locations.
	UIShaderInfo.IDColorLocation = ShaderSystem::GetUniformIndex(UIShaderInfo.UsedShader, "id_color");
	UIShaderInfo.ModelLocation = ShaderSystem::GetUniformIndex(UIShaderInfo.UsedShader, "model");
	UIShaderInfo.ProjectionLocation = ShaderSystem::GetUniformIndex(UIShaderInfo.UsedShader, "projection");
	UIShaderInfo.ViewLocation = ShaderSystem::GetUniformIndex(UIShaderInfo.UsedShader, "view");

	// Default UI properties.
	UIShaderInfo.NearClip = -100.0f;
	UIShaderInfo.FarClip = 100.0f;
	UIShaderInfo.Fov = 0;
	UIShaderInfo.ProjectionMatrix = Matrix4::Orthographic(0.0f, 1280.0f, 720.0f, 0.0f, UIShaderInfo.NearClip, UIShaderInfo.FarClip);
	UIShaderInfo.ViewMatrix = Matrix4::Identity();

	// Builtin World pick shader.
	const char* WorldShaderName = "Shader.Builtin.WorldPick";
	if (!ResourceSystem::Load(WorldShaderName, ResourceType::eResource_Type_Shader, nullptr, &ConfigResource)) {
		LOG_ERROR("Failed to load builtin UI Pick shader.");
		return false;
	}
	Config = (ShaderConfig*)ConfigResource.Data;
	if (!ShaderSystem::Create(WorlShaderInfo.Pass, Config)) {
		LOG_ERROR("Failed to load builtin UI Pick shader.");
		return false;
	}
	ResourceSystem::Unload(&ConfigResource);
	WorlShaderInfo.UsedShader = ShaderSystem::Get(WorldShaderName);

	// Extract uniform locations.
	WorlShaderInfo.IDColorLocation = ShaderSystem::GetUniformIndex(UIShaderInfo.UsedShader, "id_color");
	WorlShaderInfo.ModelLocation = ShaderSystem::GetUniformIndex(UIShaderInfo.UsedShader, "model");
	WorlShaderInfo.ProjectionLocation = ShaderSystem::GetUniformIndex(UIShaderInfo.UsedShader, "projection");
	WorlShaderInfo.ViewLocation = ShaderSystem::GetUniformIndex(UIShaderInfo.UsedShader, "view");

	// Default World properties.
	WorlShaderInfo.NearClip = 0.1f;
	WorlShaderInfo.FarClip = 1000.0f;
	WorlShaderInfo.Fov = Deg2Rad(45.0f);
	WorlShaderInfo.ProjectionMatrix = Matrix4::Perspective(WorlShaderInfo.Fov, 1280 / 720.f, WorlShaderInfo.NearClip, WorlShaderInfo.FarClip);
	WorlShaderInfo.ViewMatrix = Matrix4::Identity();

	InstanceCount = 0;
	Memory::Zero(&ColorTargetAttachment, sizeof(Texture));
	Memory::Zero(&DepthTargetAttachment, sizeof(Texture));

	if (!EngineEvent::Register(eEventCode::Mouse_Moved, this, OnMouseMoved)) {
		LOG_ERROR("Unable to listen for mouse moved event, creation failed.");
		return false;
	}

	if (!EngineEvent::Register(eEventCode::Default_Rendertarget_Refresh_Required, this, RenderViewPickOnEvent)) {
		LOG_ERROR("Unable to listen for refresh required event, creation failed.");
		return false;
	}

	LOG_INFO("Renderview pick created.");
	return true;
}

void RenderViewPick::OnDestroy() {
	EngineEvent::Unregister(eEventCode::Default_Rendertarget_Refresh_Required, this, RenderViewPickOnEvent);
	EngineEvent::Unregister(eEventCode::Mouse_Moved, this, RenderViewPickOnEvent);

	ReleaseShaderInstance();
	Renderer->DestroyTexture(&ColorTargetAttachment);
	Renderer->DestroyTexture(&DepthTargetAttachment);
}

void RenderViewPick::OnResize(uint32_t width, uint32_t height) {
	// Check if different. If so, regenerate projection matrix.
	if (width == Width && height == Height) {
		return;
	}

	Width = width;
	Height = height;

	// UI
	UIShaderInfo.ProjectionMatrix = Matrix4::Orthographic(0.0f, (float)Width, (float)Height, 0.0f, UIShaderInfo.NearClip, UIShaderInfo.FarClip);

	// World
	float Aspect = (float)Width / Height;
	WorlShaderInfo.ProjectionMatrix = Matrix4::Perspective(WorlShaderInfo.Fov, Aspect, WorlShaderInfo.NearClip, WorlShaderInfo.FarClip);

	for (uint32_t i = 0; i < RenderpassCount; ++i) {
		Passes[i].SetRenderArea(Vec4(0, 0, (float)Width, (float)Height));
	}
}

bool RenderViewPick::OnBuildPacket(void* data, struct RenderViewPacket* out_packet) {
	if (data == nullptr || out_packet == nullptr) {
		LOG_WARN("RenderViewUI::OnBuildPacke() Requires valid pointer to packet and data.");
		return false;
	}

	PickPacketData* PacketData = (PickPacketData*)data;
	out_packet->view = this;

	// TODO: Get active camera.
	Camera* WorldCamera = CameraSystem::GetDefault();
	WorlShaderInfo.ViewMatrix = WorldCamera->GetViewMatrix();

	// Set the pick packet data to extended data.
	PacketData->UIGeometryCount = 0;
	out_packet->extended_data = Memory::Allocate(sizeof(PickPacketData), MemoryType::eMemory_Type_Renderer);

	uint32_t WorldGeometryCount = (uint32_t)PacketData->WorldMeshData.size();

	uint32_t HighestInstanceID = 0;
	// Iterate all geometries in world data.
	for (uint32_t i = 0; i < WorldGeometryCount; ++i) {
		out_packet->geometries.push_back(PacketData->WorldMeshData[i]);

		// Count all geometries as a single id.
		if (PacketData->WorldMeshData[i].uniqueID > HighestInstanceID) {
			HighestInstanceID = PacketData->WorldMeshData[i].uniqueID;
		}
	}

	// Iterate all meshes in UI data.
	for (uint32_t i = 0; i < PacketData->UIMeshData.mesh_count; ++i) {
		Mesh* m = PacketData->UIMeshData.meshes[i];
		for (uint32_t j = 0; j < m->geometry_count; j++) {
			GeometryRenderData RenderData;
			RenderData.geometry = m->geometries[j];
			RenderData.model = m->Transform.GetWorldTransform();
			RenderData.uniqueID = m->UniqueID;
			out_packet->geometries.push_back(RenderData);
			PacketData->UIGeometryCount++;
		}

		// Count all geometries as a single id.
		if (m->UniqueID > HighestInstanceID) {
			HighestInstanceID = m->UniqueID;
		}
	}

	// Count texts as well.
	for (uint32_t i = 0; i < PacketData->TextCount; ++i) {
		if (PacketData->Texts[i]->UniqueID > HighestInstanceID) {
			HighestInstanceID = PacketData->Texts[i]->UniqueID;
		}
	}

	uint32_t RequiredInstanceCount = HighestInstanceID + 1;

	// TODO: this needs to take into account the highest id, not the count, because they can and do skip ids.
	// Verify instance resources exist.
	if (RequiredInstanceCount > (uint32_t)InstanceCount) {
		uint32_t Diff = RequiredInstanceCount - InstanceCount;
		for (uint32_t i = 0; i < Diff; ++i) {
			AcquireShaderInstance();
		}
	}

	// Copy over the packet data.
	Memory::Copy(out_packet->extended_data, PacketData, sizeof(PickPacketData));

	return true;
}

void RenderViewPick::OnDestroyPacket(struct RenderViewPacket* packet) {
	// No much to do here, just zero mem.
	for (uint32_t i = 0; i < packet->geometry_count; ++i) {
		packet->geometries[i].geometry = nullptr;
	}
	packet->geometries.clear();
	std::vector<GeometryRenderData>().swap(packet->geometries);

	if (packet->extended_data) {
		PickPacketData* PacketData = (PickPacketData*)packet->extended_data;
		if (!PacketData->WorldMeshData.empty()) {
			/*Memory::Free(PacketData->WorldMeshData.meshes, sizeof(Mesh) * 10, MemoryType::eMemory_Type_Array);*/
			/*PacketData->WorldMeshData.clear();
			std::vector<GeometryRenderData>().swap(PacketData->WorldMeshData);*/
		}

		Memory::Free(packet->extended_data, sizeof(PickPacketData), eMemory_Type_Renderer);
		packet->extended_data = nullptr;
	}

	Memory::Zero(packet, sizeof(RenderViewPacket));
}

bool RenderViewPick::RegenerateAttachmentTarget(uint32_t passIndex, RenderTargetAttachment* attachment) {
	if (attachment->type == eRender_Target_Attachment_Type_Color) {
		attachment->texture = &ColorTargetAttachment;
	}
	else if (attachment->type == eRender_Target_Attachment_Type_Depth) {
		attachment->texture = &DepthTargetAttachment;
	}
	else {
		LOG_ERROR("Unsupported attachment type 0x%x.", attachment->type);
		return false;
	}

	if (passIndex == 1) {
		// Do not need to regenerate for both passes since they both use the same attachment.
		// Just attach it and move on.
		return true;
	}

	// Destroy current attachment if it exists.
	if (attachment->texture){
		Renderer->DestroyTexture(attachment->texture);
	}

	// Setup a new texture.
	// Generate a UUID to act as the texture name.
	UID TextureNameUID;
	uint32_t Width = (uint32_t)Passes[passIndex].GetRenderArea().z;
	uint32_t Height = (uint32_t)Passes[passIndex].GetRenderArea().w;
	bool HasTransparency = false;

	attachment->texture->Id = INVALID_ID;
	attachment->texture->Type = TextureType::eTexture_Type_2D;
	strncpy(attachment->texture->Name, TextureNameUID.Value.c_str(), TEXTURE_NAME_MAX_LENGTH);
	attachment->texture->Width = Width;
	attachment->texture->Height = Height;
	attachment->texture->ChannelCount = 4;
	attachment->texture->Generation = INVALID_ID;
	attachment->texture->Flags |= HasTransparency ? TextureFlagBits::eTexture_Flag_Has_Transparency : 0;
	attachment->texture->Flags |= TextureFlagBits::eTexture_Flag_Is_Writeable;
	if (attachment->type == RenderTargetAttachmentType::eRender_Target_Attachment_Type_Depth) {
		attachment->texture->Flags |= TextureFlagBits::eTexture_Flag_Depth;
	}
	attachment->texture->InternalData = nullptr;

	Renderer->CreateWriteableTexture(attachment->texture);

	return true;
}

bool RenderViewPick::OnRender(struct RenderViewPacket* packet, IRendererBackend* back_renderer, size_t frame_number, size_t render_target_index) {
	uint32_t p = 0;
	IRenderpass* Pass = (IRenderpass*)&Passes[p];	// first

	if (render_target_index == 0) {
		// Reset.
		size_t Count = InstanceUpdated.size();
		for (uint32_t i = 0; i < Count; ++i) {
			InstanceUpdated[i] = false;
		}

		Pass->Begin(&Pass->Targets[render_target_index]);
		PickPacketData* PacketData = (PickPacketData*)packet->extended_data;

		int CurrentInstanceID = 0;

		// World
		if (!ShaderSystem::UseByID(WorlShaderInfo.UsedShader->ID)) {
			LOG_ERROR("Failed to use world pick shader. Render frame failed.");
			return false;
		}

		// Apply globals
		if (!ShaderSystem::SetUniformByIndex(WorlShaderInfo.ProjectionLocation, &WorlShaderInfo.ProjectionMatrix)) {
			LOG_ERROR("Failed to apply projection matrix");
		}
		if (!ShaderSystem::SetUniformByIndex(WorlShaderInfo.ViewLocation, &WorlShaderInfo.ViewMatrix)) {
			LOG_ERROR("Failed to apply view matrix");
		}
		ShaderSystem::ApplyGlobal();

		// Draw geometries. Start from 0 since world geometries are added first, and stop at the world geometry count.
		uint32_t WorldGeometryCount = (uint32_t)PacketData->WorldMeshData.size();
		for (uint32_t i = 0; i < WorldGeometryCount; ++i) {
			GeometryRenderData* Geo = &packet->geometries[i];
			CurrentInstanceID = Geo->uniqueID;

			ShaderSystem::BindInstance(CurrentInstanceID);

			// Get color based on id
			Vec3 IDColor;
			uint32_t R, G, B;
			UInt2RGB(Geo->uniqueID, &R, &G, &B);
			RGB2Vec(R, G, B, &IDColor);
			if (!ShaderSystem::SetUniformByIndex(WorlShaderInfo.IDColorLocation, &IDColor)) {
				LOG_ERROR("Failed to apply id colour uniform.");
				return false;
			}

			bool NeedsUpdate = !InstanceUpdated[CurrentInstanceID];
			ShaderSystem::ApplyInstance(NeedsUpdate);
			InstanceUpdated[CurrentInstanceID] = true;

			// Apply the locals.
			if (!ShaderSystem::SetUniformByIndex(WorlShaderInfo.ModelLocation, &Geo->model)) {
				LOG_ERROR("Failed to apply model matrix for world geometry.");
			}

			// Draw
			Renderer->DrawGeometry(&packet->geometries[i]);
		}

		Pass->End();

		p++;
		Pass = &Passes[p];

		// Second pass
		Pass->Begin(&Pass->Targets[render_target_index]);

		// UI
		if (!ShaderSystem::UseByID(UIShaderInfo.UsedShader->ID)) {
			LOG_ERROR("Failed to use material shader. Render frame failed.");
			return false;
		}

		// Apply globals.
		if (!ShaderSystem::SetUniformByIndex(UIShaderInfo.ProjectionLocation, &UIShaderInfo.ProjectionMatrix)) {
			LOG_ERROR("Failed to apply projection matrix");
		}
		if (!ShaderSystem::SetUniformByIndex(UIShaderInfo.ViewLocation, &UIShaderInfo.ViewMatrix)) {
			LOG_ERROR("Failed to apply view matrix");
		}
		ShaderSystem::ApplyGlobal();

		// Draw geometries. Start off where world geometries left off.
		for (uint32_t i = WorldGeometryCount; i < packet->geometry_count; ++i) {
			GeometryRenderData* Geo = &packet->geometries[i];
			CurrentInstanceID = Geo->uniqueID;

			ShaderSystem::BindInstance(CurrentInstanceID);

			// Get color based on id
			Vec3 IDColor;
			uint32_t R, G, B;
			UInt2RGB(Geo->uniqueID, &R, &G, &B);
			RGB2Vec(R, G, B, &IDColor);
			if (!ShaderSystem::SetUniformByIndex(WorlShaderInfo.IDColorLocation, &IDColor)) {
				LOG_ERROR("Failed to apply id colour uniform.");
				return false;
			}

			bool NeedsUpdate = !InstanceUpdated[CurrentInstanceID];
			ShaderSystem::ApplyInstance(NeedsUpdate);
			InstanceUpdated[CurrentInstanceID] = true;

			// Apply the locals.
			if (!ShaderSystem::SetUniformByIndex(WorlShaderInfo.ModelLocation, &Geo->model)) {
				LOG_ERROR("Failed to apply model matrix for world geometry.");
			}

			// Draw
			Renderer->DrawGeometry(&packet->geometries[i]);
		}

		// Draw bitmap text.
 		for (uint32_t i = 0; i < PacketData->TextCount; ++i) {
			UIText* Text = PacketData->Texts[i];
			CurrentInstanceID = Text->UniqueID;
			ShaderSystem::BindInstance(CurrentInstanceID);

			// Get color based on id
			Vec3 IDColor;
			uint32_t R, G, B;
			UInt2RGB(Text->UniqueID, &R, &G, &B);
			RGB2Vec(R, G, B, &IDColor);
			if (!ShaderSystem::SetUniformByIndex(UIShaderInfo.IDColorLocation, &IDColor)) {
				LOG_ERROR("Failed to apply id colour uniform.");
				return false;
			}

			ShaderSystem::ApplyInstance(true);

			// Apply the locals.
			Matrix4 Model = Text->Trans.GetWorldTransform();
			if (!ShaderSystem::SetUniformByIndex(UIShaderInfo.ModelLocation, &Model)) {
				LOG_ERROR("Failde to apply model matrix for text.");
			}

			Text->Draw();
		}

		Pass->End();
	}

	// Read pixel data.
	Texture* t = &ColorTargetAttachment;
	// Read the pixel at the mouse coordinate.
	unsigned char PixelRGBA[4] = { 0 };
	unsigned char* Pixel = &PixelRGBA[0];

	// Clamp to image size.
	unsigned short CoordX = CLAMP(MouseX, 0, Width - 1);
	unsigned short CoordY = CLAMP(MouseY, 0, Height - 1);
	Renderer->ReadTexturePixel(t, CoordX, CoordY, &Pixel);

	// Extract the id from the sampled color.
	uint32_t ID = INVALID_ID;
	RGB2Uint(Pixel[0], Pixel[1], Pixel[2], &ID);
	if (ID == 0x00FFFFFF) {
		// This is pure white.
		ID = INVALID_ID;
	}

	SEventContext Context;
	Context.data.u32[0] = ID;
	EngineEvent::Fire(eEventCode::Object_Hover_ID_Changed, 0, Context);

	return true;
}
