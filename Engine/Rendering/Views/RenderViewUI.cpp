#include "RenderViewUI.hpp"

#include "Core/EngineLogger.hpp"
#include "Core/DMemory.hpp"
#include "Core/Event.hpp"
#include "Math/DMath.hpp"
#include "Framework/Components/TransformComponent.h"
#include "Containers/TArray.hpp"
#include "Systems/MaterialSystem.h"
#include "Systems/ShaderSystem.h"
#include "Systems/ResourceSystem.h"
#include "Systems/RenderViewSystem.hpp"
#include "Rendering/Renderer.hpp"
#include "Rendering/Interface/IRenderpass.hpp"
#include "Rendering/Interface/IRendererBackend.hpp"
#include "Framework/Classes/TextActor.h"
#include "Framework/Components/StaticMeshComponent.h"

static bool RenderViewUIOnEvent(eEventCode code, void* sender, void* listenerInst, SEventContext context) {
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

RenderViewUI::RenderViewUI() {}

RenderViewUI::RenderViewUI(const RenderViewConfig& config) {
	Type = config.type;
	Name = config.name;
	CustomShaderName = config.custom_shader_name;
	RenderpassCount = config.pass_count;
	Passes.resize(RenderpassCount);
}

bool RenderViewUI::OnCreate(const RenderViewConfig& config) {
	// Builtin ui shader.
	const char* ShaderName = "Shader.Builtin.UI";
	UAsset ConfigResource;
	if (!ResourceSystem::Get().Load(ShaderName, EAssetType::Shader, nullptr, &ConfigResource)) {
		GLOG(Log::eError, "Failed to load builtin UI shader.");
		return false;
	}

	ShaderConfig* Config = (ShaderConfig*)ConfigResource.Data;
	// NOTE: Assuming the first pass since that's all this view has.
	if (!ShaderSystem::Get().Create(&Passes[0], Config)) {
		GLOG(Log::eError, "Failed to load builtin UI shader.");
		return false;
	}
	ResourceSystem::Get().Unload(&ConfigResource);

	UsedShader = ShaderSystem::Get().Get(CustomShaderName.IsEmpty() ? ShaderName : CustomShaderName);
	DiffuseMapLocation = UsedShader->GetUniformIndex("diffuse_texture");
	DiffuseColorLocation = UsedShader->GetUniformIndex("diffuse_color");
	ModelLocation = UsedShader->GetUniformIndex("model");

	// TODO: Set from configurable.
	NearClip = -100.0f;
	FarClip = 100.0f;

	// Default
	ProjectionMatrix = Matrix4::Orthographic(0, config.width, config.height, 0.0f, NearClip, FarClip);
	ViewMatrix = Matrix4::Identity();

	if (!EngineEvent::Register(eEventCode::Default_Rendertarget_Refresh_Required, this, RenderViewUIOnEvent)) {
		GLOG(Log::eError, "Unable to listen for refresh required event, creation failed.");
		return false;
	}

	GLOG(Log::eInfo, "Renderview ui created.");
	return true;
}

void RenderViewUI::OnDestroy() {
	EngineEvent::Unregister(eEventCode::Default_Rendertarget_Refresh_Required, this, RenderViewUIOnEvent);
}

void RenderViewUI::OnResize(uint32_t width, uint32_t height) {
	// Check if different. If so, regenerate projection matrix.
	if (width == Width && height == Height) {
		return;
	}

	Width = (uint16_t)width;
	Height = (uint16_t)height;
	ProjectionMatrix = Matrix4::Orthographic(0.0f, (float)Width, (float)Height, 0.0f, NearClip, FarClip);

	for (uint32_t i = 0; i < RenderpassCount; ++i) {
		Passes[i].SetRenderArea(Vector4(0, 0, (float)Width, (float)Height));
	}
}

bool RenderViewUI::OnBuildPacket(IRenderviewPacketData* data, struct RenderViewPacket* out_packet) {
	if (data == nullptr || out_packet == nullptr) {
		GLOG(Log::eWarn, "RenderViewUI::OnBuildPacke() Requires valid pointer to packet and data.");
		return false;
	}

	UIPacketData* PacketData = (UIPacketData*)data;
	out_packet->view = this;

	// Set matrix, etc.
	out_packet->projection_matrix = ProjectionMatrix;
	out_packet->view_matrix = ViewMatrix;

	// TODO: Temp set extended data to the test text objects for now.
	out_packet->extended_data = NewObject<UIPacketData>(*PacketData);

	// Obtain all geometries from the current scene.
	// Iterate all meshes and them to the packet's geometries collection.
	for (uint32_t i = 0; i < PacketData->meshData.mesh_count; ++i) {
		UStaticMeshComponent* MeshComp = PacketData->meshData.meshes[i]->GetComponent<UStaticMeshComponent>();
		if (!MeshComp) {
			continue;
		}

		MeshComp->DrawMesh();
		/*out_packet->geometries.push_back(RenderData);
		out_packet->geometry_count++;*/
	}

	return true;
}

void RenderViewUI::OnDestroyPacket(struct RenderViewPacket* packet) {
	// No much to do here, just zero mem.
	packet->geometries.clear();
	std::vector<GeometryRenderData>().swap(packet->geometries);

	if (packet->extended_data) {
		UIPacketData* PacketData = (UIPacketData*)packet->extended_data;
		if (PacketData->Textes != nullptr) {
			Memory::Free(PacketData->Textes, MemoryType::eMemory_Type_Array);
			PacketData->Textes = nullptr;
		}

		if (PacketData->meshData.meshes != nullptr) {
			Memory::Free(PacketData->meshData.meshes, MemoryType::eMemory_Type_Array);
			PacketData->meshData.meshes = nullptr;
		}

		DeleteObject(packet->extended_data);
		packet->extended_data = nullptr;
	}

	Memory::Zero(packet, sizeof(RenderViewPacket));
}

bool RenderViewUI::RegenerateAttachmentTarget(uint32_t passIndex, RenderTargetAttachment* attachment) {
	return true;
}

bool RenderViewUI::OnRender(struct RenderViewPacket* packet, RHI* back_renderer, size_t frame_number, size_t render_target_index) {
	std::vector<DrawCall> UIDrawCalls;
	// UI draw calls.
	for (uint32_t i = 0; i < packet->geometry_count; ++i) {
		GeometryRenderData* SrcData = &packet->geometries[i];
		if (!SrcData->geometry) continue;

		DrawCall dc;
		UMaterialInstance* Mat = SrcData->geometry->GetMaterialInstance();
		dc.geometry = SrcData->geometry;
		dc.model = SrcData->model_mat;
		dc.material = Mat;
		dc.shader = UsedShader;
		dc.userData = nullptr;
		dc.sortKey = ((uint64_t)dc.shader->ID << 32) | (uint64_t)Mat->GetInternalID();

		UIDrawCalls.push_back(dc);
	}

	// Text draw calls.
	UIPacketData* PacketData = (UIPacketData*)packet->extended_data;
	for (uint32_t i = 0; i < PacketData->textCount; ++i) {
		ATextActor* Text = PacketData->Textes[i];
		if (!Text) continue;
		UTextComponent* TextComp = Text->GetTextComponent();
		if (!TextComp) continue;

		UGeometry* TextGeometry = TextComp->GetGeometry();
		if (!TextGeometry) continue;

		DrawCall dc;
		UMaterialInstance* Mat = TextGeometry->GetMaterialInstance();
		dc.geometry = TextGeometry;
		dc.model = Text->GetLocalTransform();
		dc.material = Mat;
		dc.shader = UsedShader;
		dc.userData = nullptr;
		dc.sortKey = ((uint64_t)dc.shader->ID << 32) | (uint64_t)Mat->GetInternalID();
		UIDrawCalls.push_back(dc);

		//// TODO: font color
		//Vector4 FontColor = TextComp->GetColor();
		//if (!UsedShader->SetUniformByIndex(DiffuseColorLocation, &FontColor)) {
		//	GLOG(Log::eError, "Failed to apply bitmap font diffuse color uniform.");
		//	return false;
		//}
	}

	std::sort(UIDrawCalls.begin(), UIDrawCalls.end(),
		[](const DrawCall& a, const DrawCall& b) {
			return a.sortKey < b.sortKey;
		});

	FrameData UIData;
	UIData.projection = packet->projection_matrix;
	UIData.view = packet->view_matrix;
	UIData.ambieantColor = Vector4(0.0f);
	UIData.cameraPosition = Vector3(0.0f);
	UIData.renderMode = render_mode;
	UIData.time = packet->global_time;

	Passes[0].Begin(&Passes[0].Targets[render_target_index]);
	back_renderer->ExecuteDrawCalls(UIDrawCalls, frame_number, UIData);
	Passes[0].End();

	return true;
}
