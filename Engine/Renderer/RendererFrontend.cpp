#include "RendererFrontend.hpp"
#include "Vulkan/VulkanBackend.hpp"

#include "Core/EngineLogger.hpp"
#include "Core/DMemory.hpp"
#include "Core/Event.hpp"

#include "Math/MathTypes.hpp"
#include "Systems/MaterialSystem.h"
#include "Systems/ResourceSystem.h"
#include "Systems/ShaderSystem.h"
#include "Systems/TextureSystem.h"
#include "Systems/CameraSystem.h"

// TODO: temp
#include "Core/Event.hpp"
// TODO: temp

IRenderer::IRenderer() : Backend(nullptr), WorldRenderpass(nullptr),UIRenderpass(nullptr){}

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

	// Default framebuffer size. Overriden when window is created.
	FramebufferWidth = 1280;
	FramebufferHeight = 720;
	Resizing = false;
	FrameSinceResize = 0;

	RenderBackendConfig RendererConfig;
	RendererConfig.application_name = application_name;
	RendererConfig.OnRenderTargetRefreshRequired = std::bind(&IRenderer::RegenerateRenderTargets, this);

	// Renderpasses. TODO: Read config from file.
	RendererConfig.renderpass_count = 2;
	const char* WorldPassName = "Renderpass.Builtin.World";
	const char* UIPassName = "Renderpass.Builtin.UI";
	RenderpassConfig PassConfigs[2];
	PassConfigs[0].name = WorldPassName;
	PassConfigs[0].prev_name = nullptr;
	PassConfigs[0].next_name = UIPassName;
	PassConfigs[0].render_area = Vec4(0.0f, 0.0f, 1280.0f, 720.0f);
	PassConfigs[0].clear_color = Vec4(0.0f, 0.0f, 0.2f, 1.0f);
	PassConfigs[0].clear_flags = RenderpassClearFlags::eRenderpass_Clear_Color_Buffer | RenderpassClearFlags::eRenderpass_Clear_Depth_Buffer | RenderpassClearFlags::eRenderpass_Clear_Stencil_Buffer;

	PassConfigs[1].name = UIPassName;
	PassConfigs[1].prev_name = WorldPassName;
	PassConfigs[1].next_name = nullptr;
	PassConfigs[1].render_area = Vec4(0.0f, 0.0f, 1280.0f, 720.0f);
	PassConfigs[1].clear_color = Vec4(0.0f, 0.0f, 0.2f, 1.0f);
	PassConfigs[1].clear_flags = RenderpassClearFlags::eRenderpass_Clear_None;

	RendererConfig.pass_config = PassConfigs;

	if (!Backend->Initialize(&RendererConfig, &WindowRenderTargetCount, plat_state)) {
		UL_FATAL("Renderer backend init failed.");
		return false;
	}

	// TODO: Will know how to get these when we define views.
	WorldRenderpass = Backend->GetRenderpass(WorldPassName);
	WorldRenderpass->RenderTargetCount = WindowRenderTargetCount;
	WorldRenderpass->Targets.resize(WindowRenderTargetCount);

	UIRenderpass = Backend->GetRenderpass(UIPassName);
	UIRenderpass->RenderTargetCount = WindowRenderTargetCount;
	UIRenderpass->Targets.resize(WindowRenderTargetCount);

	RegenerateRenderTargets();

	// Update the main/world renderpass dimensions.
	WorldRenderpass->SetRenderArea(Vec4(0, 0, (float)FramebufferWidth, (float)FramebufferHeight));
	UIRenderpass->SetRenderArea(Vec4(0, 0, (float)FramebufferWidth, (float)FramebufferHeight));

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

		for (unsigned char i = 0; i < WindowRenderTargetCount; ++i) {
			Backend->DestroyRenderTarget(&WorldRenderpass->Targets[i], true);
			Backend->DestroyRenderTarget(&UIRenderpass->Targets[i], true);
		}

		Backend->Shutdown();
		Memory::Free(Backend, sizeof(IRendererBackend), eMemory_Type_Renderer);
	}

	Backend = nullptr;
}

void IRenderer::OnResize(unsigned short width, unsigned short height) {
	if (Backend != nullptr) {
		Resizing = true;
		FramebufferWidth = width;
		FramebufferHeight = height;
		FrameSinceResize = 0;
	}
	else {
		UL_WARN("Renderer backend does not exist to accept resize: %i %i", width, height);
	}
}

bool IRenderer::DrawFrame(SRenderPacket* packet) {
	Backend->IncreaseFrameNum();

	// Make sure the window is not currently being resized by waiting a designated
	// number of frames after the last resize operation before performing the backend updates.
	if (Resizing) {
		FrameSinceResize++;

		// If the required number of frames have passed since the resize, go ahead and perform the actual update.
		if (FrameSinceResize >= 30) {
			float Width = (float)FramebufferWidth;
			float Height = (float)FramebufferHeight;
			Projection = Matrix4::Perspective(Deg2Rad(45.0f), Width / (float)Height, NearClip, FarClip, true);
			UIProjection = Matrix4::Orthographic(0, (float)Width, (float)Height, 0, -100.0f, 100.0f);
			Backend->Resize(
				static_cast<unsigned short>(Width), 
				static_cast<unsigned short>(Height)
			);

			FrameSinceResize = 0;
			Resizing = false;
		}
		else {
			// Skip rendering the frame and try again next time.
			return true;
		}
	}

	// TOD): Views
	WorldRenderpass->SetRenderArea(Vec4(0, 0, (float)FramebufferWidth, (float)FramebufferHeight));
	UIRenderpass->SetRenderArea(Vec4(0, 0, (float)FramebufferWidth, (float)FramebufferHeight));

	// Camera
	if (ActiveWorldCamera == nullptr) {
		ActiveWorldCamera = CameraSystem::GetDefault();
	}

	Matrix4 View = ActiveWorldCamera->GetViewMatrix();
	Vec3 ViewPosition = ActiveWorldCamera->GetPosition();

	if (Backend->BeginFrame(packet->delta_time)) {
		unsigned char AttachmentIndex = Backend->GetWindowAttachmentIndex();

		// World render pass.
		if (!Backend->BeginRenderpass(WorldRenderpass, &WorldRenderpass->Targets[AttachmentIndex])) {
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
		
		if (!Backend->EndRenderpass(WorldRenderpass)) {
			UL_ERROR("Backend end eButilin_Renderpass_World renderpass failed. Application quit now.");
			return false;
		}
		// End world renderpass

		// UI renderpass
		if (!Backend->BeginRenderpass(UIRenderpass, &UIRenderpass->Targets[AttachmentIndex])) {
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

		if (!Backend->EndRenderpass(UIRenderpass)) {
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

void IRenderer::CreateWriteableTexture(Texture* tex) {
	Backend->CreateWriteableTexture(tex);
}

void IRenderer::ResizeTexture(Texture* tex, uint32_t new_width, uint32_t new_height) {
	Backend->ResizeTexture(tex, new_width, new_height);
}

void IRenderer::WriteTextureData(Texture* tex, uint32_t offset, uint32_t size, const unsigned char* pixels) {
	Backend->WriteTextureData(tex, offset, size, pixels);
}

bool IRenderer::CreateGeometry(Geometry* geometry, uint32_t vertex_size, uint32_t vertex_count,
	const void* vertices, uint32_t index_size, uint32_t index_count, const void* indices) {
	return Backend->CreateGeometry(geometry, vertex_size, vertex_count, vertices, index_size, index_count, indices);
}

void IRenderer::DestroyGeometry(Geometry* geometry) {
	Backend->DestroyGeometry(geometry);
}

IRenderpass* IRenderer::GetRenderpass(const char* name) {
	return Backend->GetRenderpass(name);
}

bool IRenderer::CreateRenderShader(Shader* shader, IRenderpass* pass, unsigned short stage_count, std::vector<char*> stage_filenames, std::vector<ShaderStage> stages) {
	return Backend->CreateShader(shader, pass, stage_count, stage_filenames, stages);
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

uint32_t IRenderer::AcquireInstanceResource(Shader* shader, std::vector<TextureMap*> maps) {
	return Backend->AcquireInstanceResource(shader, maps);
}

bool IRenderer::ReleaseInstanceResource(Shader* shader, uint32_t instance_id) {
	return Backend->ReleaseInstanceResource(shader, instance_id);
}

bool IRenderer::SetUniform(Shader* shader, ShaderUniform* uniform, const void* value) {
	return Backend->SetUniform(shader, uniform, value);
}

bool IRenderer::AcquireTextureMap(TextureMap* map) {
	return Backend->AcquireTextureMap(map);
}

void IRenderer::ReleaseTextureMap(TextureMap* map) {
	Backend->ReleaseTextureMap(map);
}

void IRenderer::CreateRenderTarget(unsigned char attachment_count, std::vector<Texture*> attachments, IRenderpass* pass, uint32_t width, uint32_t height, RenderTarget* out_target) {
	Backend->CreateRenderTarget(attachment_count, attachments, pass, width, height, out_target);
}

void IRenderer::DestroyRenderTarget(RenderTarget* target, bool free_internal_memory) {
	Backend->DestroyRenderTarget(target, free_internal_memory);
}

void IRenderer::CreateRenderpass(IRenderpass* out_renderpass, float depth, uint32_t stencil, bool has_prev_pass, bool has_next_pass) {
	Backend->CreateRenderpass(out_renderpass, depth, stencil, has_prev_pass, has_next_pass);
}

void IRenderer::DestroyRenderpass(IRenderpass* pass) {
	Backend->DestroyRenderpass(pass);
}

void IRenderer::RegenerateRenderTargets() {
	// Create render targets for each. TODO: Should be configurable.
	for (unsigned char i = 0; i < WindowRenderTargetCount; ++i) {
		// Destroy the old first if exists.
		Backend->DestroyRenderTarget(&WorldRenderpass->Targets[i], false);
		Backend->DestroyRenderTarget(&UIRenderpass->Targets[i], false);

		Texture* WindowTargetTexture = Backend->GetWindowAttachment(i);
		Texture* DepthTargetTexture = Backend->GetDepthAttachment();

		// World render targets.
		std::vector<Texture*> Attachments = {WindowTargetTexture, DepthTargetTexture};
		Backend->CreateRenderTarget(2, Attachments, WorldRenderpass, FramebufferWidth, FramebufferHeight, &WorldRenderpass->Targets[i]);

		// UI render targets.
		std::vector<Texture*> UIAttachments = { WindowTargetTexture };
		Backend->CreateRenderTarget(1, UIAttachments, UIRenderpass, FramebufferWidth, FramebufferHeight, &UIRenderpass->Targets[i]);

	}
}
