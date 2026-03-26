#pragma once

#include "Rendering/Interface/IRendererBackend.hpp"
#include "VulkanContext.hpp"

class VulkanBuffer;

class VulkanRHI : public RHI {
	friend class VulkanShader;
	friend class VulkanBuffer;
	friend class VulkanTexture;

public:
	VulkanRHI();
	virtual ~VulkanRHI();

public:
	virtual bool Initialize(const RenderBackendConfig* config, unsigned char* out_window_render_target_count, struct SPlatformState* plat_state) override;
	virtual void Shutdown() override;

	virtual bool BeginFrame(double delta_time) override;
	virtual void DrawGeometry(GeometryRenderData* geometry) override;
	virtual bool EndFrame(double delta_time) override;
	virtual void Resize(unsigned short width, unsigned short height) override;

	// Textures
	virtual UTexture* AcquireTexture(const FString& name, bool auto_release) override;

	virtual bool CreateGeometry(Geometry* geometry, uint32_t vertex_size, uint32_t vertex_count, 
		const void* vertices, uint32_t index_size, uint32_t index_count, const void* indices) override;
	virtual void DestroyGeometry(Geometry* geometry) override;

	// Renderpass
	virtual bool BeginRenderpass(IRenderpass* pass, RenderTarget* target) override;
	virtual bool EndRenderpass(IRenderpass* pass) override;
	virtual bool CreateRenderTarget(unsigned char attachment_count, std::vector<RenderTargetAttachment> attachments, IRenderpass* pass, uint32_t width, uint32_t height, RenderTarget* out_target) override;
	virtual void DestroyRenderTarget(RenderTarget* target, bool free_internal_memory) override;
	virtual UTexture* GetWindowAttachment(unsigned char index) override;
	virtual unsigned char GetWindowAttachmentCount() const override;
	virtual UTexture* GetDepthAttachment(unsigned char index) override;
	virtual unsigned char GetWindowAttachmentIndex() override;
	virtual bool CreateRenderpass(IRenderpass* out_renderpass, const RenderpassConfig& config) override;
	virtual void DestroyRenderpass(IRenderpass* pass) override;

	// Renderbuffer
	virtual bool DrawRenderbuffer(IGPUBuffer* buffer, size_t offset, uint32_t element_count, bool bind_only) override;

	// Render target
	virtual void SetViewport(const Vector4& rect) override;
	virtual void ResetViewport() override;
	virtual void SetScissor(const Vector4& rect) override;
	virtual void ResetScissor() override;

	// Shaders.
	virtual bool CreateShader(Shader* shader, const ShaderConfig* config, IRenderpass* pass, const TArray<FString>& stage_filenames, std::vector<ShaderStage>& stages) override;
	virtual uint32_t AcquireInstanceResource(Shader* shader, std::vector<TextureMap*>& maps) override;
	virtual bool ReleaseInstanceResource(Shader* shader, uint64_t instance_id) override;

	virtual bool AcquireTextureMap(TextureMap* map) override;
	virtual void ReleaseTextureMap(TextureMap* map) override;
	virtual bool GetEnabledMultiThread() const override;

public:
	virtual void CreateCommandBuffer();
	virtual bool RecreateSwapchain();

	virtual bool VerifyShaderID(uint32_t shader_id);

private:
	uint32_t QueryInstanceVersion();
	uint32_t SelectTargetApiVersion(uint32_t instanceVersion);

private:
	vk::SamplerAddressMode ConvertRepeatType(const char* axis, TextureRepeat repeat);
	vk::Filter ConvertFilterType(const char* op, TextureFilter filter);

protected:
	VulkanContext Context;

};