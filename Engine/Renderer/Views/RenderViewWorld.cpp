#include "RenderViewWorld.hpp"

#include "Core/EngineLogger.hpp"
#include "Core/Event.hpp"
#include "Core/DMemory.hpp"
#include "Math/DMath.hpp"
#include "Math/Transform.hpp"
#include "Containers/TArray.hpp"
#include "Containers/TString.hpp"

#include "Systems/MaterialSystem.h"
#include "Systems/ShaderSystem.h"
#include "Systems/CameraSystem.h"
#include "Systems/ResourceSystem.h"
#include "Systems/RenderViewSystem.hpp"

#include "Renderer/RendererFrontend.hpp"
#include "Renderer/Interface/IRenderpass.hpp"
#include "Renderer/Interface/IRendererBackend.hpp"

struct GeometryDistance {
	GeometryRenderData g;
	float distance;
};

static void QuickSort(std::vector<GeometryDistance>& arr, int low_index, int high_index, bool ascending);

static bool RenderViewWorldOnEvent(eEventCode code, void* sender, void* listenerInst, SEventContext context) {
	IRenderView* self = (IRenderView*)listenerInst;
	if (self == nullptr) {
		return false;
	}

	switch ((eEventCode)code)
	{
	case eEventCode::Default_Rendertarget_Refresh_Required: 
	{
		RenderViewSystem::RegenerateRendertargets(self);
		return true;
	}

	case eEventCode::Set_Render_Mode:
	{
		int RenderMode = context.data.i32[0];
		switch (RenderMode)
		{
		case ShaderRenderMode::eShader_Render_Mode_Default:
			self->render_mode = ShaderRenderMode::eShader_Render_Mode_Default;
			LOG_DEBUG("Change render mode: eShader_Render_Mode_Default.");
			break;

		case ShaderRenderMode::eShader_Render_Mode_Lighting:
			self->render_mode = ShaderRenderMode::eShader_Render_Mode_Lighting;
			LOG_DEBUG("Change render mode: eShader_Render_Mode_Lighting.");
			break;

		case ShaderRenderMode::eShader_Render_Mode_Normals:
			self->render_mode = ShaderRenderMode::eShader_Render_Mode_Normals;
			LOG_DEBUG("Change render mode: eShader_Render_Mode_Normals.");
			break;

		}
		return true;
	}
        default: return true;
	}	// switch


	return false;
}

bool ReloadShader(eEventCode code, void* sender, void* listenerInst, SEventContext context) {
	RenderViewWorld* self = (RenderViewWorld*)listenerInst;
	if (self == nullptr) {
		return false;
	}

	// Builtin world shader.
	const char* ShaderName = "Shader.Builtin.World";
	Resource ConfigResource;
	if (!ResourceSystem::Load(ShaderName, ResourceType::eResource_Type_Shader, nullptr, &ConfigResource)) {
		LOG_ERROR("Failed to load builtin skybox shader.");
		return false;
	}

	ShaderConfig* Config = (ShaderConfig*)ConfigResource.Data;
	// NOTE: Assuming the first pass since that's all this view has.
	if (!ShaderSystem::Create(&self->GetRenderpass()[0], Config)) {
		LOG_ERROR("Failed to load builtin world shader.");
		return false;
	}
	ResourceSystem::Unload(&ConfigResource);

	Shader* s = ShaderSystem::Get(ShaderName);
	self->SetShader(s);
	RenderViewSystem::RegenerateRendertargets(self);

	return true;
}

RenderViewWorld::RenderViewWorld() {
	
}

RenderViewWorld::RenderViewWorld(const RenderViewConfig& config) {
	Type = config.type;
	Name = StringCopy(config.name);
	CustomShaderName = config.custom_shader_name;
	RenderpassCount = config.pass_count;
	Passes.resize(RenderpassCount);
}

bool RenderViewWorld::OnCreate(const RenderViewConfig& config) {
	// Builtin world shader.
	const char* ShaderName = "Shader.Builtin.World";
	Resource ConfigResource;
	if (!ResourceSystem::Load(ShaderName, ResourceType::eResource_Type_Shader, nullptr, &ConfigResource)) {
		LOG_ERROR("Failed to load builtin skybox shader.");
		return false;
	}

	ShaderConfig* Config = (ShaderConfig*)ConfigResource.Data;
	// NOTE: Assuming the first pass since that's all this view has.
	if (!ShaderSystem::Create(&Passes[0], Config)) {
		LOG_ERROR("Failed to load builtin world shader.");
		return false;
	}
	ResourceSystem::Unload(&ConfigResource);

	UsedShader = ShaderSystem::Get(CustomShaderName ? CustomShaderName : ShaderName);
	ReserveY = true;

	// TODO: Set from configurable.
	NearClip = 0.1f;
	FarClip = 1000.0f;
	Fov = Deg2Rad(45.0f);

	// Default
	ProjectionMatrix = Matrix4::Perspective(Fov, 1280.0f / 720.0f, NearClip, FarClip);
	WorldCamera = CameraSystem::GetDefault();

	// TODO: Obtain from scene.
	AmbientColor = Vec4(0.7f, 0.7f, 0.7f, 1.0f);

	if (!EngineEvent::Register(eEventCode::Default_Rendertarget_Refresh_Required, this, RenderViewWorldOnEvent)) {
		LOG_ERROR("Unable to listen for refresh required event, creation failed.");
		return false;
	}
	if (!EngineEvent::Register(eEventCode::Reload_Shader_Module, this, ReloadShader)) {
		LOG_ERROR("Unable to listen for refresh required event, creation failed.");
		return false;
	}
	if (!EngineEvent::Register(eEventCode::Set_Render_Mode, this, RenderViewWorldOnEvent)) {
		LOG_ERROR("Unable to listen for refresh required event, creation failed.");
		return false;
	}
	
	LOG_INFO("Renderview world created.");
	return true;
}

void RenderViewWorld::OnDestroy() {
	EngineEvent::Unregister(eEventCode::Default_Rendertarget_Refresh_Required, this, RenderViewWorldOnEvent);
	EngineEvent::Unregister(eEventCode::Reload_Shader_Module, this, ReloadShader);
	EngineEvent::Unregister(eEventCode::Set_Render_Mode, this, RenderViewWorldOnEvent);
}

void RenderViewWorld::OnResize(uint32_t width, uint32_t height) {
	// Check if different. If so, regenerate projection matrix.
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

bool RenderViewWorld::OnBuildPacket(void* data, struct RenderViewPacket* out_packet) {
	if (data == nullptr || out_packet == nullptr) {
		LOG_WARN("RenderViewUI::OnBuildPacke() Requires valid pointer to packet and data.");
		return false;
	}

	std::vector<GeometryRenderData>* GeometryData = (std::vector<GeometryRenderData>*)data;
	out_packet->view = this;

	// Set matrix, etc.
	out_packet->projection_matrix = ProjectionMatrix;
	out_packet->view_matrix = WorldCamera->GetViewMatrix();
	out_packet->view_position = WorldCamera->GetPosition();
	out_packet->ambient_color = AmbientColor;

	// Obtain all geometries from the current scene.

	std::vector<GeometryDistance> GeometryDistances;

	uint32_t GeometryDataCount = (uint32_t)GeometryData->size();
	for (uint32_t i = 0; i < GeometryDataCount; ++i) {
		GeometryRenderData* GData = &(*GeometryData)[i];
		if (GData->geometry == nullptr) {
			continue;
		}

		// TODO: Add something to material to check for transparency.
		if ((GData->geometry->Material->DiffuseMap.texture->Flags & TextureFlagBits::eTexture_Flag_Has_Transparency) == 0) {
			// Only add meshes with _no_ transparency.
			out_packet->geometries.push_back((*GeometryData)[i]);
			out_packet->geometry_count++;
		}
		else {
			// For meshes _with_ transparency, add them to a separate list to be sorted by distance later.
			// Get the center, extract the global position from the model matrix and add it to the center,
			// then calculate the distance between it and the camera, and finally save it to a list to be sorted.
			// NOTE: This isn't perfect for translucent meshes that intersect, but is enough for our purposes now.
			Vec3 Center = (*GeometryData)[i].geometry->Center.Transform(GData->model);
			float Distance = Center.Distance(WorldCamera->GetPosition());

			GeometryDistance gDist;
			gDist.distance = Dabs(Distance);
			gDist.g = (*GeometryData)[i];

			GeometryDistances.push_back(gDist);
		}
	}

	// Sort the distances.
	uint32_t GeometryCount = (uint32_t)GeometryDistances.size();
	QuickSort(GeometryDistances, 0, GeometryCount - 1, true);

	// Add them to packet geometry.
	for (uint32_t i = 0; i < GeometryCount; ++i) {
		out_packet->geometries.push_back(GeometryDistances[i].g);
		out_packet->geometry_count++;
	}

	GeometryDistances.clear();
	std::vector<GeometryDistance>().swap(GeometryDistances);

	return true;
}


void RenderViewWorld::OnDestroyPacket(struct RenderViewPacket* packet) {
	// No much to do here, just zero mem.
	packet->geometries.clear();
	std::vector<GeometryRenderData>().swap(packet->geometries);

	Memory::Zero(packet, sizeof(RenderViewPacket));
}

bool RenderViewWorld::RegenerateAttachmentTarget(uint32_t passIndex, RenderTargetAttachment* attachment) {
	return true;
}

bool RenderViewWorld::OnRender(struct RenderViewPacket* packet, IRendererBackend* back_renderer, size_t frame_number, size_t render_target_index) {
	uint32_t SID = UsedShader->ID;
	for (uint32_t p = 0; p < RenderpassCount; ++p) {
		IRenderpass* Pass = (IRenderpass*)&Passes[p];
		Pass->Begin(&Pass->Targets[render_target_index]);

		if (!ShaderSystem::UseByID(SID)) {
			LOG_ERROR("RenderViewUI::OnRender() Failed to use material shader. Render frame failed.");
			return false;
		}

		// Apply globals.
		if (!MaterialSystem::ApplyGlobal(SID, frame_number, packet->projection_matrix, packet->view_matrix, packet->ambient_color, packet->view_position, render_mode)) {
			LOG_ERROR("RenderViewUI::OnRender() Failed to use global shader. Render frame failed.");
			return false;
		}

		// Draw geometries.
		uint32_t Count = packet->geometry_count;
		for (uint32_t i = 0; i < Count; ++i) {
			Material* Mat = nullptr;
			if (packet->geometries[i].geometry->Material) {
				Mat = packet->geometries[i].geometry->Material;
			}
			else {
				Mat = MaterialSystem::GetDefaultMaterial();
			}

			// Update the material if it hasn't already been this frame. This keeps the
			// same material from being updated multiple times. It still needs to be bound
			// either way, so this check result gets passed to the backend which either
			// updates the internal shader bindings and binds them, or only binds them.
			bool IsNeedUpdate = Mat->RenderFrameNumer != frame_number;
			if (!MaterialSystem::ApplyInstance(Mat, IsNeedUpdate)) {
				LOG_WARN("Failed to apply material '%s'. Skipping draw.", Mat->Name);
				continue;
			}
			else {
				// Sync the frame number.
				Mat->RenderFrameNumer = (uint32_t)frame_number;
			}

			// Apply local
			MaterialSystem::ApplyLocal(Mat, packet->geometries[i].model);

			// Draw
			back_renderer->DrawGeometry(&packet->geometries[i]);
		}

		Pass->End();
	}

	return true;
}


// Quick sort geometry distance.
static void Swap(GeometryDistance* a, GeometryDistance* b) {
	GeometryDistance temp = *a;
	*a = *b;
	*b = temp;
}

static int Partition(std::vector<GeometryDistance>& arr, int low_index, int high_index, bool ascending) {
	GeometryDistance Privot = arr[high_index];
	int i = (low_index - 1);

	for (int j = low_index; j <= high_index; j++) {
		if (ascending) {
			if (arr[j].distance < Privot.distance) {
				++i;
				Swap(&arr[i], &arr[j]);
			}
		}
		else {
			if (arr[j].distance > Privot.distance) {
				++i;
				Swap(&arr[i], &arr[j]);
			}
		}
	}

	Swap(&arr[i + 1], &arr[high_index]);
	return i + 1;
}

static void QuickSort(std::vector<GeometryDistance>& arr, int low_index, int high_index, bool ascending) {
	if (low_index < high_index) {
		int PartitionIndex = Partition(arr, low_index, high_index, ascending);

		// Independently sort elements before and after the partition index.
		QuickSort(arr, low_index, PartitionIndex - 1, ascending);
		QuickSort(arr, PartitionIndex + 1, high_index, ascending);
	}
}
