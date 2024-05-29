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

struct GeometryDistance {
	GeometryRenderData g;
	float distance;
};

static void QuickSort(TArray<GeometryDistance> arr, int low_index, int high_index, bool ascending);


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
	AmbientColor = Vec4(0.7f, 0.7f, 0.7f, 1.0f);
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

	TArray<GeometryDistance> GeometryDistances;

	for (uint32_t i = 0; i < MeshData->mesh_count; ++i) {
		Mesh* pMesh = &MeshData->meshes[i];
		Matrix4 Model = pMesh->Transform.GetWorldTransform();

		for (uint32_t j = 0; j < pMesh->geometry_count; j++) {
			GeometryRenderData RenderData;
			RenderData.geometry = pMesh->geometries[j];
			RenderData.model = Model;

			if ((pMesh->geometries[j]->Material->DiffuseMap.texture->Flags & TextureFlagBits::eTexture_Flag_Has_Transparency) == 0) {
				// Only add meshes with _no_ transparency.
				out_packet->geometries.push_back(RenderData);
				out_packet->geometry_count++;
			}
			else {
				// For meshes _with_ transparency, add them to a separate list to be sorted by distance later.
				// Get the center, extract the global position from the model matrix and add it to the center,
				// then calculate the distance between it and the camera, and finally save it to a list to be sorted.
				// NOTE: This isn't perfect for translucent meshes that intersect, but is enough for our purposes now.
				Vec3 Center = RenderData.geometry->Center.Transform(Model);
				float Distance = Center.Distance(WorldCamera->GetPosition());

				GeometryDistance gDist;
				gDist.distance = Dabs(Distance);
				gDist.g = RenderData;

				GeometryDistances.Push(gDist);
			}
		}
	}

	// Sort the distances.
	uint32_t GeometryCount = (uint32_t)GeometryDistances.Size();
	QuickSort(GeometryDistances, 0, GeometryCount - 1, true);

	// Add them to packet geometry.
	for (uint32_t i = 0; i < GeometryCount; ++i) {
		out_packet->geometries.push_back(GeometryDistances[i].g);
		out_packet->geometry_count++;
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
		if (!MaterialSystem::ApplyGlobal(SID, frame_number, packet->projection_matrix, packet->view_matrix, packet->ambient_color, packet->view_position)) {
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


// Quick sort geometry distance.
static void Swap(GeometryDistance* a, GeometryDistance* b) {
	GeometryDistance temp = *a;
	*a = *b;
	*b = temp;
}

static int Partition(TArray<GeometryDistance> arr, int low_index, int high_index, bool ascending) {
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

static void QuickSort(TArray<GeometryDistance> arr, int low_index, int high_index, bool ascending) {
	if (low_index < high_index) {
		int PartitionIndex = Partition(arr, low_index, high_index, ascending);

		// Independently sort elements before and after the partition index.
		QuickSort(arr, low_index, PartitionIndex - 1, ascending);
		QuickSort(arr, PartitionIndex + 1, high_index, ascending);
	}
}