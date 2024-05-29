#include "RenderViewWorld.hpp"

#include "Core/EngineLogger.hpp"
#include "Core/DMemory.hpp"
#include "Core/Event.hpp"
#include "Math/DMath.hpp"
#include "Math/Transform.hpp"
#include "Containers/TArray.hpp"

#include "Systems/MaterialSystem.h"
#include "Systems/ShaderSystem.h"
#include "Systems/CameraSystem.h"

#include "Renderer/RendererFrontend.hpp"
#include "Renderer/Interface/IRenderpass.hpp"
#include "Renderer/Interface/IRendererBackend.hpp"


void RenderViewWorld::OnCreate() {
	ShaderID = ShaderSystem::GetID(CustomShaderName ? CustomShaderName : "Shader.Builtin.Material");
	ReserveY = true;

	// TODO: Set from configurable.
	NearClip = 0.1f;
	FarClip = 1000.0f;
	Fov = Deg2Rad(45.0f);

	// Default
	ProjectionMatrix = Matrix4::Perspective(Fov, 1280.0f / 720.0f, NearClip, FarClip, ReserveY);
	WorldCamera = CameraSystem::GetDefault();

	// TODO: Obtain from scene.
	AmbientColor = Vec4(0.85f, 0.85f, 0.85f, 1.0f);
}

void RenderViewWorld::OnDestroy() {

}

void RenderViewWorld::OnResize(uint32_t width, uint32_t height) {
	// Check if different. If so, regenerate projection matrix.
	if (width == Width && height == Height) {
		return;
	}

	Width = width;
	Height = height;
	ProjectionMatrix = Matrix4::Perspective(Fov, (float)Width / (float)Height, NearClip, FarClip, ReserveY);

	for (uint32_t i = 0; i < RenderpassCount; ++i) {
		Passes[i]->SetRenderArea(Vec4(0, 0, (float)Width, (float)Height));
	}
}

bool RenderViewWorld::OnBuildPacket(void* data, struct RenderViewPacket* out_packet) const {
	if (data == nullptr || out_packet == nullptr) {
		UL_WARN("RenderViewUI::OnBuildPacke() Requires valid pointer to packet and data.");
		return false;
	}

	MeshPacketData* MeshData = (MeshPacketData*)data;
	out_packet->view = this;

	// Set matrix, etc.
	out_packet->projection_matrix = ProjectionMatrix;
	out_packet->view_matrix = WorldCamera->GetViewMatrix();
	out_packet->view_position = WorldCamera->GetPosition();
	out_packet->ambient_color = AmbientColor;

	// Obtain all geometries from the current scene.
	// Iterate all meshes and them to the packet's geometries collection.
	for (uint32_t i = 0; i < MeshData->mesh_count; ++i) {
		Mesh* pMesh = &MeshData->meshes[i];
		for (uint32_t j = 0; j < pMesh->geometry_count; j++) {
			if ((pMesh->geometries[j]->Material->DiffuseMap.texture->Flags & TextureFlagBits::eTexture_Flag_Has_Transparency) == 0) {
				GeometryRenderData RenderData;
				RenderData.geometry = pMesh->geometries[j];
				RenderData.model = pMesh->Transform.GetWorldTransform();
				out_packet->geometries.push_back(RenderData);
				out_packet->geometry_count++;
			}
		}
	}

	return true;
}

bool RenderViewWorld::OnRender(struct RenderViewPacket* packet, IRendererBackend* back_renderer, size_t frame_number, size_t render_target_index) const {
	uint32_t SID = ShaderID;
	for (uint32_t p = 0; p < RenderpassCount; ++p) {
		IRenderpass* Pass = Passes[p];
		Pass->Begin(&Pass->Targets[render_target_index]);

		if (!ShaderSystem::UseByID(SID)) {
			UL_ERROR("RenderViewUI::OnRender() Failed to use material shader. Render frame failed.");
			return false;
		}

		// Apply globals.
		if (!MaterialSystem::ApplyGlobal(SID, packet->projection_matrix, packet->view_matrix, packet->ambient_color, packet->view_position)) {
			UL_ERROR("RenderViewUI::OnRender() Failed to use global shader. Render frame failed.");
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

			bool IsNeedUpdate = Mat->RenderFrameNumer != frame_number;
			if (!MaterialSystem::ApplyInstance(Mat, IsNeedUpdate)) {
				UL_WARN("Failed to apply material '%s'. Skipping draw.", Mat->Name);
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

