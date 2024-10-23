#include "RendererFrontend.hpp"
#include "Vulkan/VulkanBackend.hpp"
#include "Interface/IRenderbuffer.hpp"

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

IRenderer::IRenderer() : Backend(nullptr){}

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

	if (!Backend->Initialize(&RendererConfig, &WindowRenderTargetCount, plat_state)) {
		LOG_FATAL("Renderer backend init failed.");
		return false;
	}

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

			Backend->Resize(
				static_cast<unsigned short>(Width), 
				static_cast<unsigned short>(Height)
			);

			RenderViewSystem::OnWindowResize(FramebufferWidth, FramebufferHeight);

			FrameSinceResize = 0;
			Resizing = false;
		}
		else {
			// Skip rendering the frame and try again next time.
			//Platform::PlatformSleep(16);
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

void IRenderer::SetViewport(Vec4 rect) {
	Backend->SetViewport(rect);
}

void IRenderer::ResetViewport() {
	Backend->ResetViewport();
}

void IRenderer::SetScissor(Vec4 rect) {
	Backend->SetScissor(rect);
}

void IRenderer::ResetScissor() {
	Backend->ResetScissor();
}

void IRenderer::CreateTexture(const unsigned char* pixels, Texture* texture) {
	Backend->CreateTexture(pixels, texture);
}

void IRenderer::DestroyTexture(Texture* texture) {
	Backend->DestroyTexture(texture);
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

bool IRenderer::CreateRenderShader(Shader* shader, const ShaderConfig* config, IRenderpass* pass, std::vector<char*> stage_filenames, std::vector<ShaderStage> stages) {
	return Backend->CreateShader(shader, config, pass, stage_filenames, stages);
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

void IRenderer::ReadTextureData(Texture* tex, uint32_t offset, uint32_t size, void** outMemeory) {
	Backend->ReadTextureData(tex, offset, size, outMemeory);
}

void IRenderer::ReadTexturePixel(Texture* tex, uint32_t x, uint32_t y, unsigned char** outRGBA) {
	Backend->ReadTexturePixel(tex, x, y, outRGBA);
}

bool IRenderer::CreateRenderTarget(unsigned char attachment_count, std::vector<RenderTargetAttachment> attachments, IRenderpass* pass, uint32_t width, uint32_t height, RenderTarget* out_target) {
	return Backend->CreateRenderTarget(attachment_count, attachments, pass, width, height, out_target);
}

void IRenderer::DestroyRenderTarget(RenderTarget* target, bool free_internal_memory) {
	Backend->DestroyRenderTarget(target, free_internal_memory);
}

Texture* IRenderer::GetWindowAttachment(unsigned char index) {
	return Backend->GetWindowAttachment(index);
}

unsigned char IRenderer::GetWindowAttachmentCount() const {
	return Backend->GetWindowAttachmentCount();
}

Texture* IRenderer::GetDepthAttachment(unsigned char index) {
	return Backend->GetDepthAttachment(index);
}

unsigned char IRenderer::GetWindowAttachmentIndex() {
	return Backend->GetWindowAttachmentIndex();
}

bool IRenderer::CreateRenderpass(IRenderpass* out_renderpass, const RenderpassConfig* config) {
	if (config == nullptr) {
		LOG_ERROR("Renderpass config is required.");
		return false;
	}

	if (config->renderTargetCount == 0) {
		LOG_ERROR("Can not have a renderpass target count of0.");
		return false;
	}

	return Backend->CreateRenderpass(out_renderpass, config);
}

void IRenderer::DestroyRenderpass(IRenderpass* pass) {
	Backend->DestroyRenderpass(pass);
}

bool IRenderer::GetEnabledMutiThread() const {
	return Backend->GetEnabledMultiThread();
}

/**
 * Renderbuffer
 */
bool IRenderer::CreateRenderbuffer(enum RenderbufferType type, size_t total_size, bool use_freelist, IRenderbuffer * buffer) {
	return Backend->CreateRenderbuffer(type, total_size, use_freelist, buffer);
}

void IRenderer::DestroyRenderbuffer(IRenderbuffer* buffer) {
	Backend->DestroyRenderbuffer(buffer);
}

bool IRenderer::BindRenderbuffer(IRenderbuffer* buffer, size_t offset) {
	if (buffer == nullptr) {
		LOG_ERROR("IRenderer::BindRenderbuffer() requires a valid pointer to a buffer.");
		return false;
	}

	return Backend->BindRenderbuffer(buffer, offset);
}

bool IRenderer::UnBindRenderbuffer(IRenderbuffer* buffer) {
	return Backend->UnBindRenderbuffer(buffer);
}

void* IRenderer::MapMemory(IRenderbuffer* buffer, size_t offset, size_t size) {
	return Backend->MapMemory(buffer, offset, size);
}

void IRenderer::UnmapMemory(IRenderbuffer* buffer, size_t offset, size_t size) {
	Backend->UnmapMemory(buffer, offset, size);
}

bool IRenderer::FlushRenderbuffer(IRenderbuffer* buffer, size_t offset, size_t size) {
	return Backend->FlushRenderbuffer(buffer, offset, size);
}

bool IRenderer::ReadRenderbuffer(IRenderbuffer* buffer, size_t offset, size_t size, void** out_memory) {
	return Backend->ReadRenderbuffer(buffer, offset, size, out_memory);
}

bool IRenderer::ResizeRenderbuffer(IRenderbuffer* buffer, size_t new_size) {
	return Backend->ResizeRenderbuffer(buffer, new_size);
}

bool IRenderer::AllocateRenderbuffer(IRenderbuffer* buffer, size_t size, size_t* out_offset) {
	return AllocateRenderbuffer(buffer, size, out_offset);
}

bool IRenderer::FreeRenderbuffer(IRenderbuffer* buffer, size_t size, size_t offset) {
	return Backend->FlushRenderbuffer(buffer, offset, size);
}

bool IRenderer::LoadRange(IRenderbuffer* buffer, size_t offset, size_t size, const void* data) {
	return Backend->LoadRange(buffer, offset, size, data);
}

bool IRenderer::CopyRange(IRenderbuffer* src, size_t src_offset, IRenderbuffer* dst, size_t dst_offset, size_t size) {
	return Backend->CopyRange(src, src_offset, dst, dst_offset, size);
}

bool IRenderer::DrawRenderbuffer(IRenderbuffer* buffer, size_t offset, uint32_t element_count, bool bind_only) {
	return Backend->DrawRenderbuffer(buffer, offset, element_count, bind_only);
}
