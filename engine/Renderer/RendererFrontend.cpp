#include "RendererFrontend.hpp"
#include "Vulkan/VulkanBackend.hpp"

#include "Core/EngineLogger.hpp"
#include "Core/DMemory.hpp"

#include "Math/MathTypes.hpp"
#include "Systems/MaterialSystem.h"
#include "Systems/TextureSystem.h"

// TODO: temp
#include "Core/Event.hpp"
// TODO: temp

IRenderer::IRenderer() : Backend(nullptr) {}

IRenderer::IRenderer(RendererBackendType type, struct SPlatformState* plat_state) : Backend(nullptr){
	if (plat_state == nullptr) {
		return ;
	}

	BackendType = type;
	if (type == eRenderer_Backend_Type_Vulkan) {
		// TODO: fill
		void* TempBackend = (VulkanBackend*)Memory::Allocate(sizeof(VulkanBackend), MemoryType::eMemory_Type_Renderer);
		Backend = new(TempBackend)VulkanBackend();

		// TODO: make this configurable
		Backend->SetFrameNum(0);

		return ;
	}

}

IRenderer::~IRenderer() {
	Shutdown();
}

bool IRenderer::Initialize(const char* application_name, struct SPlatformState* plat_state) {
	if (Backend == nullptr) {
		return false;
	}

	if (!Backend->Initialize(application_name, plat_state)) {
		UL_FATAL("Renderer backend init failed.");
		return false;
	}

	// World projection/view
	NearClip = 0.01f;
	FarClip = 1000.0f;
	Projection = Matrix4::Perspective(Deg2Rad(45.0f), 1280.0f / 720.0f, 0.1f, 1000.0f);
	View = Matrix4::Identity();
	View.SetTranslation(Vec3{ 0.0f, 0.0f, -10.0f });

	// UI projection/view
	UIProjection = Matrix4::Orthographic(0, 1280.0f, 720.0f, 0, -100.f, 100.f);
	UIView = Matrix4::Identity().Inverse();

	return true;
}

void IRenderer::Shutdown() {
	if (Backend != nullptr) {

		Backend->Shutdown();
		Memory::Free(Backend, sizeof(IRendererBackend), eMemory_Type_Renderer);
	}

	Backend = nullptr;
}

void IRenderer::OnResize(unsigned short width, unsigned short height) {
	if (Backend != nullptr) {
		Projection = Matrix4::Perspective(Deg2Rad(45.0f), (float)width / (float)height, NearClip, FarClip);
		UIProjection = Matrix4::Orthographic(0, (float)width, (float)height, 0, -100.f, 100.f);
		Backend->Resize(width, height);
	}
	else {
		UL_WARN("Renderer backend does not exist to accept resize: %i %i", width, height);
	}
}

bool IRenderer::DrawFrame(SRenderPacket* packet) {
	if (Backend->BeginFrame(packet->delta_time)) {
		// World render pass.
		if (!Backend->BeginRenderpass(eButilin_Renderpass_World)) {
			UL_ERROR("Backend begin eButilin_Renderpass_World renderpass failed. Application quit now.");
			return false;
		}
		
		// Update UBO buffer.
		Backend->UpdateGlobalWorldState(Projection, View, Vec3(0.0f, 0.0f, 0.0f), Vec4(1.0f, 1.0f, 1.0f, 1.0f), 0);

		// Draw geometries.
		for (uint32_t i = 0; i < packet->geometry_count; ++i) {
			Backend->DrawGeometry(packet->geometries[i]);
		}
		
		if (!Backend->EndRenderpass(eButilin_Renderpass_World)) {
			UL_ERROR("Backend end eButilin_Renderpass_World renderpass failed. Application quit now.");
			return false;
		}
		// End world renderpass

		// UI renderpass
		if (!Backend->BeginRenderpass(eButilin_Renderpass_UI)) {
			UL_ERROR("Backend begin eButilin_Renderpass_UI renderpass failed. Application quit now.");
			return false;
		}

		// Update UI buffer.
		Backend->UpdateGlobalUIState(UIProjection, UIView, 0);

		// Draw geometries.
		for (uint32_t i = 0; i < packet->ui_geometry_count; ++i) {
			Backend->DrawGeometry(packet->ui_geometries[i]);
		}

		if (!Backend->EndRenderpass(eButilin_Renderpass_UI)) {
			UL_ERROR("Backend end eButilin_Renderpass_UI renderpass failed. Application quit now.");
			return false;
		}
		// End UI renderpass

		// End frame
		bool result = Backend->EndFrame(packet->delta_time);
		Backend->IncreaseFrameNum();

		if (!result) {
			UL_ERROR("Renderer end frame failed.");
			return false;
		}

	}

	return true;
}

void IRenderer::CreateTexture(Texture* texture) {
	Memory::Zero(texture, sizeof(Texture));
	texture->Generation = INVALID_ID;
}

void IRenderer::CreateTexture(const unsigned char* pixels, Texture* texture) {
	Backend->CreateTexture(pixels, texture);
}

void IRenderer::DestroyTexture(Texture* txture) {
	Backend->DestroyTexture(txture);
}


bool IRenderer::CreateMaterial(Material* material) {
	return Backend->CreateMaterial(material);
}

void IRenderer::DestroyMaterial(Material* material) {
	Backend->DestroyMaterial(material);
}

bool IRenderer::CreateGeometry(Geometry* geometry, uint32_t vertex_count,
	const Vertex* vertices, uint32_t index_count, const uint32_t* indices) {
	return Backend->CreateGeometry(geometry, vertex_count, vertices, index_count, indices);
}

void IRenderer::DestroyGeometry(Geometry* geometry) {
	Backend->DestroyGeometry(geometry);
}