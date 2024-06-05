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
#include "Systems/RenderViewSystem.hpp"

// TODO: temp
#include "Core/Event.hpp"
// TODO: temp

IRenderer::IRenderer() : Backend(nullptr), WorldRenderpass(nullptr),UIRenderpass(nullptr){}

IRenderer::IRenderer(RendererBackendType type, struct SPlatformState* plat_state) : Backend(nullptr){
	IRenderer();

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
	RendererConfig.renderpass_count = 3;
	const char* SkyboxPassName = "Renderpass.Builtin.Skybox";
	const char* WorldPassName = "Renderpass.Builtin.World";
	const char* UIPassName = "Renderpass.Builtin.UI";
	RenderpassConfig PassConfigs[3];
	PassConfigs[0].name = SkyboxPassName;
	PassConfigs[0].prev_name = nullptr;
	PassConfigs[0].next_name = WorldPassName;
	PassConfigs[0].render_area = Vec4(0.0f, 0.0f, 1280.0f, 720.0f);
	PassConfigs[0].clear_color = Vec4(0.0f, 0.0f, 0.2f, 1.0f);
	PassConfigs[0].clear_flags = RenderpassClearFlags::eRenderpass_Clear_Color_Buffer;

	PassConfigs[1].name = WorldPassName;
	PassConfigs[1].prev_name = SkyboxPassName;
	PassConfigs[1].next_name = UIPassName;
	PassConfigs[1].render_area = Vec4(0.0f, 0.0f, 1280.0f, 720.0f);
	PassConfigs[1].clear_color = Vec4(0.0f, 0.0f, 0.2f, 1.0f);
	PassConfigs[1].clear_flags = RenderpassClearFlags::eRenderpass_Clear_Depth_Buffer | RenderpassClearFlags::eRenderpass_Clear_Stencil_Buffer;

	PassConfigs[2].name = UIPassName;
	PassConfigs[2].prev_name = WorldPassName;
	PassConfigs[2].next_name = nullptr;
	PassConfigs[2].render_area = Vec4(0.0f, 0.0f, 1280.0f, 720.0f);
	PassConfigs[2].clear_color = Vec4(0.0f, 0.0f, 0.2f, 1.0f);
	PassConfigs[2].clear_flags = RenderpassClearFlags::eRenderpass_Clear_None;

	RendererConfig.pass_config = PassConfigs;

	if (!Backend->Initialize(&RendererConfig, &WindowRenderTargetCount, plat_state)) {
		LOG_FATAL("Renderer backend init failed.");
		return false;
	}

	// TODO: Will know how to get these when we define views.
	SkyboxRenderpass = Backend->GetRenderpass(SkyboxPassName);
	SkyboxRenderpass->RenderTargetCount = WindowRenderTargetCount;
	SkyboxRenderpass->Targets.resize(WindowRenderTargetCount);

	WorldRenderpass = Backend->GetRenderpass(WorldPassName);
	WorldRenderpass->RenderTargetCount = WindowRenderTargetCount;
	WorldRenderpass->Targets.resize(WindowRenderTargetCount);

	UIRenderpass = Backend->GetRenderpass(UIPassName);
	UIRenderpass->RenderTargetCount = WindowRenderTargetCount;
	UIRenderpass->Targets.resize(WindowRenderTargetCount);

	RegenerateRenderTargets();

	// Update the main/world renderpass dimensions.
	SkyboxRenderpass->SetRenderArea(Vec4(0, 0, (float)FramebufferWidth, (float)FramebufferHeight));
	WorldRenderpass->SetRenderArea(Vec4(0, 0, (float)FramebufferWidth, (float)FramebufferHeight));
	UIRenderpass->SetRenderArea(Vec4(0, 0, (float)FramebufferWidth, (float)FramebufferHeight));

	// Shaders
	Resource ConfigResource;
	ShaderConfig* Config = nullptr;

	// Builtin skybox shader.
	ResourceSystem::Load(BUILTIN_SHADER_NAME_SKYBOX, ResourceType::eResource_Type_Shader, nullptr, &ConfigResource);
	Config = (ShaderConfig*)ConfigResource.Data;
	ShaderSystem::Create(Config);
	ResourceSystem::Unload(&ConfigResource);
	SkyboxShaderID = ShaderSystem::GetID(BUILTIN_SHADER_NAME_SKYBOX);
	Config = nullptr;

	// Builtin material shader.
	ResourceSystem::Load(BUILTIN_SHADER_NAME_MATERIAL, ResourceType::eResource_Type_Shader, nullptr, &ConfigResource);
	Config = (ShaderConfig*)ConfigResource.Data;
	ShaderSystem::Create(Config);
	ResourceSystem::Unload(&ConfigResource);
	MaterialShaderID = ShaderSystem::GetID(BUILTIN_SHADER_NAME_MATERIAL);
	Config = nullptr;

	// Builtin ui shader.
	ResourceSystem::Load(BUILTIN_SHADER_NAME_UI, ResourceType::eResource_Type_Shader, nullptr, &ConfigResource);
	Config = (ShaderConfig*)ConfigResource.Data;
	ShaderSystem::Create(Config);
	ResourceSystem::Unload(&ConfigResource);
	UISHaderID = ShaderSystem::GetID(BUILTIN_SHADER_NAME_UI);
	Config = nullptr;

	return true;
}

void IRenderer::Shutdown() {
	if (Backend != nullptr) {

		for (unsigned char i = 0; i < WindowRenderTargetCount; ++i) {
			Backend->DestroyRenderTarget(&SkyboxRenderpass->Targets[i], true);
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
		LOG_WARN("Renderer backend does not exist to accept resize: %i %i", width, height);
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

			RenderViewSystem::OnWindowResize(FramebufferWidth, FramebufferHeight);

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

	if (Backend->BeginFrame(packet->delta_time)) {
		unsigned char AttachmentIndex = Backend->GetWindowAttachmentIndex();

		// Render each view.
		for (uint32_t i = 0; i < packet->view_count; ++i) {
			if (!RenderViewSystem::OnRender(packet->views[i].view, &packet->views[i], Backend->GetFrameNum(), AttachmentIndex)) {
				LOG_ERROR("Error rendering view index '%i'.", i);
				return false;
			}
		}

		// End frame
		bool result = Backend->EndFrame(packet->delta_time);

		if (!result) {
			LOG_ERROR("Renderer end frame failed.");
			return false;
		}

	}

	return true;
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

void IRenderer::DrawGeometry(GeometryRenderData* data) {
	Backend->DrawGeometry(data);
}

bool IRenderer::BeginRenderpass(IRenderpass* pass, RenderTarget* target) {
	return Backend->BeginRenderpass(pass, target);
}

bool IRenderer::EndRenderpass(IRenderpass* pass) {
	return Backend->EndRenderpass(pass);
}

IRenderpass* IRenderer::GetRenderpass(const char* name) {
	return Backend->GetRenderpass(name);
}

bool IRenderer::CreateRenderShader(Shader* shader, const ShaderConfig* config, IRenderpass* pass, unsigned short stage_count, std::vector<char*> stage_filenames, std::vector<ShaderStage> stages) {
	return Backend->CreateShader(shader, config, pass, stage_count, stage_filenames, stages);
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
		Backend->DestroyRenderTarget(&SkyboxRenderpass->Targets[i], false);
		Backend->DestroyRenderTarget(&WorldRenderpass->Targets[i], false);
		Backend->DestroyRenderTarget(&UIRenderpass->Targets[i], false);

		Texture* WindowTargetTexture = Backend->GetWindowAttachment(i);
		Texture* DepthTargetTexture = Backend->GetDepthAttachment();

		// Skybox render targets.
		std::vector<Texture*> SkyboxAttachments = { WindowTargetTexture };
		Backend->CreateRenderTarget(1, SkyboxAttachments, SkyboxRenderpass, FramebufferWidth, FramebufferHeight, &SkyboxRenderpass->Targets[i]);

		// World render targets.
		std::vector<Texture*> WorldAttachments = {WindowTargetTexture, DepthTargetTexture};
		Backend->CreateRenderTarget(2, WorldAttachments, WorldRenderpass, FramebufferWidth, FramebufferHeight, &WorldRenderpass->Targets[i]);

		// UI render targets.
		std::vector<Texture*> UIAttachments = { WindowTargetTexture };
		Backend->CreateRenderTarget(1, UIAttachments, UIRenderpass, FramebufferWidth, FramebufferHeight, &UIRenderpass->Targets[i]);

	}
}

bool IRenderer::GetEnabledMutiThread() const {
	return Backend->GetEnabledMultiThread();
}