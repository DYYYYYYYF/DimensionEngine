#include "Renderer.hpp"
#include "Vulkan/VulkanBackend.hpp"
#include "Interface/IGPUBuffer.hpp"

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

IRenderer* IRenderer::Renderer = nullptr;

IRenderer::IRenderer() {
	RHI_ = nullptr;
	BackendType = RendererBackendType::eRenderer_Backend_Type_Vulkan;
	WindowRenderTargetCount = 0;
	FramebufferWidth = 1920;
	FramebufferHeight = 1080;
	Resizing = false;
	FrameSinceResize = 0;
}

IRenderer::IRenderer(RendererBackendType type, struct SPlatformState* plat_state) : RHI_(nullptr){
	BackendType = RendererBackendType::eRenderer_Backend_Type_Vulkan;
	WindowRenderTargetCount = 0;
	FramebufferWidth = 1920;
	FramebufferHeight = 1080;
	Resizing = false;
	FrameSinceResize = 0;

	if (plat_state == nullptr) {
		return ;
	}

	BackendType = type;
	if (type == eRenderer_Backend_Type_Vulkan) {
		// TODO: fill
		void* TempBackend = (VulkanRHI*)Memory::Allocate(sizeof(VulkanRHI), MemoryType::eMemory_Type_Renderer);
		RHI_ = new(TempBackend)VulkanRHI();

		// TODO: make this configurable
		RHI_->SetFrameNum(0);
	}
}

IRenderer::~IRenderer() {
	Shutdown();
}

IRenderer* IRenderer::GetRenderer() {
	if (!Renderer) {
		GLOG(Log::Level::eError, "Function IRenderer::GetRenderer() should called after initialization. Return nullptr.");
		return nullptr;
	}
	return Renderer;
}

bool IRenderer::Initialize(const std::string& application_name, Vector2 window_size, struct SPlatformState* plat_state) {
	if (RHI_ == nullptr) {
		return false;
	}

	Renderer = this;

	// Default framebuffer size. Overriden when window is created.
	FramebufferWidth = (uint32_t)window_size.x;
	FramebufferHeight = (uint32_t)window_size.y;
	Resizing = false;
	FrameSinceResize = 0;

	RenderBackendConfig RendererConfig;
	RendererConfig.application_name = application_name;

	if (!RHI_->Initialize(&RendererConfig, &WindowRenderTargetCount, plat_state)) {
		GLOG(Log::eFatal, "Renderer backend init failed.");
		return false;
	}

	return true;
}

void IRenderer::Shutdown() {
	if (RHI_ != nullptr) {
		RHI_->Shutdown();
		Memory::Free(RHI_, eMemory_Type_Renderer);
	}

	RHI_ = nullptr;
}

void IRenderer::OnResize(unsigned short width, unsigned short height) {
	if (RHI_ != nullptr) {
		Resizing = true;
		FramebufferWidth = width;
		FramebufferHeight = height;
		FrameSinceResize = 0;
	}
	else {
		GLOG(Log::eWarn, "Renderer backend does not exist to accept resize: %i %i", width, height);
	}
}

bool IRenderer::DrawFrame(SRenderPacket* packet) {
	RHI_->IncreaseFrameNum();

	// Make sure the window is not currently being resized by waiting a designated
	// number of frames after the last resize operation before performing the backend updates.
	if (Resizing) {
		FrameSinceResize++;

		// If the required number of frames have passed since the resize, go ahead and perform the actual update.
		if (FrameSinceResize >= 30) {
			float Width = (float)FramebufferWidth;
			float Height = (float)FramebufferHeight;

			RHI_->Resize(
				static_cast<unsigned short>(Width), 
				static_cast<unsigned short>(Height)
			);

			RenderViewSystem::Get().OnWindowResize(FramebufferWidth, FramebufferHeight);

			FrameSinceResize = 0;
			Resizing = false;
		}
		else {
			// Skip rendering the frame and try again next time.
			//Platform::PlatformSleep(16);
			return true;
		}
	}

	if (RHI_->BeginFrame(packet->delta_time)) {
		unsigned char AttachmentIndex = RHI_->GetWindowAttachmentIndex();

		// Render each view.
		for (uint32_t i = 0; (uint32_t)i < packet->views.size(); ++i) {
			if (!RenderViewSystem::Get().OnRender(packet->views[i].view, &packet->views[i], RHI_->GetFrameNum(), AttachmentIndex)) {
				GLOG(Log::eError, "Error rendering view index '%i'.", i);
				return false;
			}
		}

		// End frame
		bool result = RHI_->EndFrame(packet->delta_time);

		if (!result) {
			GLOG(Log::eError, "Renderer end frame failed.");
			return false;
		}

	}

	return true;
}

void IRenderer::SetViewport(Vector4 rect) {
	RHI_->SetViewport(rect);
}

void IRenderer::ResetViewport() {
	RHI_->ResetViewport();
}

void IRenderer::SetScissor(Vector4 rect) {
	RHI_->SetScissor(rect);
}

void IRenderer::ResetScissor() {
	RHI_->ResetScissor();
}

UTexture* IRenderer::AcquireTexture(const FString& name, bool auto_release) {
	return RHI_->AcquireTexture(name, auto_release);
}

bool IRenderer::CreateGeometry(Geometry* geometry, uint32_t vertex_size, uint32_t vertex_count,
	const void* vertices, uint32_t index_size, uint32_t index_count, const void* indices) {
	return RHI_->CreateGeometry(geometry, vertex_size, vertex_count, vertices, index_size, index_count, indices);
}

void IRenderer::DestroyGeometry(Geometry* geometry) {
	RHI_->DestroyGeometry(geometry);
}

void IRenderer::DrawGeometry(GeometryRenderData* data) {
	RHI_->DrawGeometry(data);
}

bool IRenderer::BeginRenderpass(IRenderpass* pass, RenderTarget* target) {
	return RHI_->BeginRenderpass(pass, target);
}

bool IRenderer::EndRenderpass(IRenderpass* pass) {
	return RHI_->EndRenderpass(pass);
}

bool IRenderer::CreateRenderShader(Shader* shader, const ShaderConfig* config, IRenderpass* pass, const TArray<FString>& stage_filenames, std::vector<ShaderStage> stages) {
	return RHI_->CreateShader(shader, config, pass, stage_filenames, stages);
}

void IRenderer::DestroyRenderShader(Shader* shader) {
	shader->Destroy();
}

bool IRenderer::InitializeRenderShader(Shader* shader) {
	return shader->Initialize();
}

uint32_t IRenderer::AcquireInstanceResource(Shader* shader, std::vector<TextureMap*> maps) {
	return RHI_->AcquireInstanceResource(shader, maps);
}

bool IRenderer::ReleaseInstanceResource(Shader* shader, uint32_t instance_id) {
	return RHI_->ReleaseInstanceResource(shader, instance_id);
}

bool IRenderer::AcquireTextureMap(TextureMap* map) {
	return RHI_->AcquireTextureMap(map);
}

void IRenderer::ReleaseTextureMap(TextureMap* map) {
	RHI_->ReleaseTextureMap(map);
}

bool IRenderer::CreateRenderTarget(unsigned char attachment_count, std::vector<RenderTargetAttachment> attachments, IRenderpass* pass, uint32_t width, uint32_t height, RenderTarget* out_target) {
	return RHI_->CreateRenderTarget(attachment_count, attachments, pass, width, height, out_target);
}

void IRenderer::DestroyRenderTarget(RenderTarget* target, bool free_internal_memory) {
	RHI_->DestroyRenderTarget(target, free_internal_memory);
}

UTexture* IRenderer::GetWindowAttachment(unsigned char index) {
	return RHI_->GetWindowAttachment(index);
}

unsigned char IRenderer::GetWindowAttachmentCount() const {
	return RHI_->GetWindowAttachmentCount();
}

UTexture* IRenderer::GetDepthAttachment(unsigned char index) {
	return RHI_->GetDepthAttachment(index);
}

unsigned char IRenderer::GetWindowAttachmentIndex() {
	return RHI_->GetWindowAttachmentIndex();
}

bool IRenderer::CreateRenderpass(IRenderpass* out_renderpass, const RenderpassConfig& config) {
	if (config.renderTargetCount == 0) {
		GLOG(Log::eError, "Can not have a renderpass target count of0.");
		return false;
	}

	return RHI_->CreateRenderpass(out_renderpass, config);
}

void IRenderer::DestroyRenderpass(IRenderpass* pass) {
	RHI_->DestroyRenderpass(pass);
}

bool IRenderer::GetEnabledMutiThread() const {
	return RHI_->GetEnabledMultiThread();
}

/**
 * Renderbuffer
 */
bool IRenderer::DrawRenderbuffer(IGPUBuffer* buffer, size_t offset, uint32_t element_count, bool bind_only) {
	return RHI_->DrawRenderbuffer(buffer, offset, element_count, bind_only);
}
