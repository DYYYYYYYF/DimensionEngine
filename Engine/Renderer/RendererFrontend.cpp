#include "RendererFrontend.hpp"
#include "Vulkan/VulkanBackend.hpp"

#include "Core/EngineLogger.hpp"
#include "Core/DMemory.hpp"

#include "Math/MathTypes.hpp"
#include "Systems/MaterialSystem.h"
#include "Systems/ResourceSystem.h"
#include "Systems/ShaderSystem.h"
#include "Systems/TextureSystem.h"
#include "Systems/CameraSystem.h"

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

	// Shaders
	Resource ConfigResource;
	ShaderConfig* Config = nullptr;
	
	// Builtin material shader.
	ResourceSystem::Load(BUILTIN_SHADER_NAME_MATERIAL, ResourceType::eResource_Type_Shader, &ConfigResource);
	Config = (ShaderConfig*)ConfigResource.Data;
	ShaderSystem::Create(Config);
	ResourceSystem::Unload(&ConfigResource);
	MaterialShaderID = ShaderSystem::GetID(BUILTIN_SHADER_NAME_MATERIAL);

	// Builtin ui shader.
	ResourceSystem::Load(BUILTIN_SHADER_NAME_UI, ResourceType::eResource_Type_Shader, &ConfigResource);
	Config = (ShaderConfig*)ConfigResource.Data;
	ShaderSystem::Create(Config);
	ResourceSystem::Unload(&ConfigResource);
	UISHaderID = ShaderSystem::GetID(BUILTIN_SHADER_NAME_UI);

	// World projection/view
	NearClip = 0.01f;
	FarClip = 1000.0f;
	Projection = Matrix4::Perspective(Deg2Rad(45.0f), 1280.0f / 720.0f, 0.1f, 1000.0f, true);

	// UI projection/view
	UIProjection = Matrix4::Orthographic(0, 1280.0f, 720.0f, 0, -100.f, 100.f);
	UIView = Matrix4::Identity();

	AmbientColor = Vec4(0.25, 0.25, 0.25, 1.0f);

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
		Projection = Matrix4::Perspective(Deg2Rad(45.0f), (float)width / (float)height, NearClip, FarClip, true);
		UIProjection = Matrix4::Orthographic(0, (float)width, (float)height, 0, -100.f, 100.f);
		Backend->Resize(width, height);
	}
	else {
		UL_WARN("Renderer backend does not exist to accept resize: %i %i", width, height);
	}
}

bool IRenderer::DrawFrame(SRenderPacket* packet) {
	Backend->IncreaseFrameNum();

	if (ActiveWorldCamera == nullptr) {
		ActiveWorldCamera = CameraSystem::GetDefault();
	}

	Matrix4 View = ActiveWorldCamera->GetViewMatrix();
	Vec3 ViewPosition = ActiveWorldCamera->GetPosition();

	if (Backend->BeginFrame(packet->delta_time)) {
		// World render pass.
		if (!Backend->BeginRenderpass(eButilin_Renderpass_World)) {
			UL_ERROR("Backend begin eButilin_Renderpass_World renderpass failed. Application quit now.");
			return false;
		}
		
		// Update UBO buffer.
		if (!ShaderSystem::UseByID(MaterialShaderID)) {
			UL_ERROR("Failed to use material shader. Render frame failed.");
			return false;
		}

		// Apply globals.
		if (!MaterialSystem::ApplyGlobal(MaterialShaderID, Projection, View, AmbientColor, ViewPosition)) {
			UL_ERROR("Failed to apply globals to material shader. Render frame failed.");
			return false;
		}

		// Draw geometries.
		for (uint32_t i = 0; i < packet->geometry_count; ++i) {
			Material* mat = nullptr;
			if (packet->geometries[i].geometry->Material) {
				mat = packet->geometries[i].geometry->Material;
			}
			else {
				mat = MaterialSystem::GetDefaultMaterial();
			}

			// Apply the material if it hasn't already been this frame. This keeps the
			// same material from being updated multiple times.
			bool IsNeedUpdate = (mat->RenderFrameNumer != Backend->GetFrameNum());
			if (!MaterialSystem::ApplyInstance(mat, IsNeedUpdate)) {
				UL_ERROR("Failed to apply material '%s'. Skipping draw.", mat->Name);
				continue;
			}
			else {
				// Sync the frame number.
				mat->RenderFrameNumer = (uint32_t)Backend->GetFrameNum();
			}

			// Apply the locals.
			MaterialSystem::ApplyLocal(mat, packet->geometries[i].model);

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
		if (!ShaderSystem::UseByID(UISHaderID)) {
			UL_ERROR("Failed to use ui shader. Render frame failed.");
			return false;
		}

		// Apply globals.
		if (!MaterialSystem::ApplyGlobal(UISHaderID, UIProjection, UIView, AmbientColor, ViewPosition)) {
			UL_ERROR("Failed to apply globals to ui shader. Render frame failed.");
			return false;
		}

		// Draw ui geometries.
		for (uint32_t i = 0; i < packet->ui_geometry_count; ++i) {
			Material* Mat = nullptr;
			if (packet->ui_geometries[i].geometry->Material) {
				Mat = packet->ui_geometries[i].geometry->Material;
			}
			else {
				Mat = MaterialSystem::GetDefaultMaterial();
			}

			// Apply the material
			bool IsNeedUpdate = (Mat->RenderFrameNumer != Backend->GetFrameNum());
			if (!MaterialSystem::ApplyInstance(Mat, IsNeedUpdate)) {
				UL_ERROR("Failed to apply ui '%s'. Skipping draw.", Mat->Name);
				continue;
			}
			else {
				Mat->RenderFrameNumer = (uint32_t)Backend->GetFrameNum();
			}

			// Apply the locals.
			MaterialSystem::ApplyLocal(Mat, packet->ui_geometries[i].model);

			Backend->DrawGeometry(packet->ui_geometries[i]);
		}

		if (!Backend->EndRenderpass(eButilin_Renderpass_UI)) {
			UL_ERROR("Backend end eButilin_Renderpass_UI renderpass failed. Application quit now.");
			return false;
		}
		// End UI renderpass

		// End frame
		bool result = Backend->EndFrame(packet->delta_time);

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

bool IRenderer::CreateGeometry(Geometry* geometry, uint32_t vertex_size, uint32_t vertex_count,
	const void* vertices, uint32_t index_size, uint32_t index_count, const void* indices) {
	return Backend->CreateGeometry(geometry, vertex_size, vertex_count, vertices, index_size, index_count, indices);
}

void IRenderer::DestroyGeometry(Geometry* geometry) {
	Backend->DestroyGeometry(geometry);
}

unsigned short IRenderer::GetRenderpassID(const char* name) {
	// TODO: HACK: Need dynamic renderpasses instead of hardcoding them.
	if (strcmp("Renderpass.Builtin.World", name) == 0) {
		return eButilin_Renderpass_World;
	}
	else if (strcmp("Renderpass.Builtin.UI", name) == 0) {
		return eButilin_Renderpass_UI;
	}

	UL_ERROR("renderer_renderpass_id: No renderpass named '%s'.", name);
	return INVALID_ID_U8;
}

bool IRenderer::CreateRenderShader(Shader* shader, unsigned short renderpass_id, unsigned short stage_count, std::vector<char*> stage_filenames, std::vector<ShaderStage> stages) {
	return Backend->CreateShader(shader, renderpass_id, stage_count, stage_filenames, stages);
}

bool IRenderer::DestroyRenderShader(Shader* shader) {
	return Backend->DestroyShader(shader);
}

bool IRenderer::InitializeRenderShader(Shader* shader) {
	return Backend->InitializeShader(shader);
}

bool IRenderer::UseRenderShader(Shader* shader) {
	return Backend->UseShader(shader);
}

bool IRenderer::BindGlobalsRenderShader(Shader* shader) {
	return Backend->BindGlobalsShader(shader);
}

bool IRenderer::BindInstanceRenderShader(Shader* shader, uint32_t instance_id) {
	return Backend->BindInstanceShader(shader, instance_id);
}

bool IRenderer::ApplyGlobalRenderShader(Shader* shader) {
	return Backend->ApplyGlobalShader(shader);
}

bool IRenderer::ApplyInstanceRenderShader(Shader* shader, bool need_update) {
	return Backend->ApplyInstanceShader(shader, need_update);
}

uint32_t IRenderer::AcquireInstanceResource(Shader* shader) {
	return Backend->AcquireInstanceResource(shader);
}

bool IRenderer::ReleaseInstanceResource(Shader* shader, uint32_t instance_id) {
	return Backend->ReleaseInstanceResource(shader, instance_id);
}

bool IRenderer::SetUniform(Shader* shader, ShaderUniform* uniform, const void* value) {
	return Backend->SetUniform(shader, uniform, value);
}