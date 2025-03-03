#pragma once

#include "Renderer/Interface/IRendererBackend.hpp"
#include "Core/DMemory.hpp"
#include "VulkanContext.hpp"

class VulkanBuffer;

class VulkanBackend : public IRendererBackend {
	friend VulkanShader;

public:
	VulkanBackend();
	virtual ~VulkanBackend();

public:
	virtual bool Initialize(const RenderBackendConfig* config, unsigned char* out_window_render_target_count, struct SPlatformState* plat_state) override;
	virtual void Shutdown() override;

	virtual bool BeginFrame(double delta_time) override;
	virtual void DrawGeometry(GeometryRenderData* geometry) override;
	virtual bool EndFrame(double delta_time) override;
	virtual void Resize(unsigned short width, unsigned short height) override;

	// Textures
	virtual void CreateTexture(const unsigned char* pixels, Texture* texture) override;
	virtual void DestroyTexture(Texture* texture) override;
	virtual void CreateWriteableTexture(Texture* tex) override;
	virtual void ResizeTexture(Texture* tex, uint32_t new_width, uint32_t new_height) override;
	virtual void WriteTextureData(Texture* tex, uint32_t offset, uint32_t size, const unsigned char* pixels) override;
	virtual void ReadTextureData(Texture* tex, uint32_t offset, uint32_t size, void** outMemeory) override;
	virtual void ReadTexturePixel(Texture* tex, uint32_t x, uint32_t y, unsigned char** outRGBA) override;

	virtual bool CreateGeometry(Geometry* geometry, uint32_t vertex_size, uint32_t vertex_count, 
		const void* vertices, uint32_t index_size, uint32_t index_count, const void* indices) override;
	virtual void DestroyGeometry(Geometry* geometry) override;

	// Renderpass
	virtual bool BeginRenderpass(IRenderpass* pass, RenderTarget* target) override;
	virtual bool EndRenderpass(IRenderpass* pass) override;
	virtual bool CreateRenderTarget(unsigned char attachment_count, std::vector<RenderTargetAttachment> attachments, IRenderpass* pass, uint32_t width, uint32_t height, RenderTarget* out_target) override;
	virtual void DestroyRenderTarget(RenderTarget* target, bool free_internal_memory) override;
	virtual Texture* GetWindowAttachment(unsigned char index) override;
	virtual unsigned char GetWindowAttachmentCount() const override;
	virtual Texture* GetDepthAttachment(unsigned char index) override;
	virtual unsigned char GetWindowAttachmentIndex() override;
	virtual bool CreateRenderpass(IRenderpass* out_renderpass, const RenderpassConfig& config) override;
	virtual void DestroyRenderpass(IRenderpass* pass) override;

	// Renderbuffer
	virtual bool CreateRenderbuffer(enum RenderbufferType type, size_t total_size, bool use_freelist, IRenderbuffer* buffer) override;
	virtual bool CreateRenderbuffer(IRenderbuffer* buffer) override;
	virtual void DestroyRenderbuffer(IRenderbuffer* buffer) override;
	virtual bool BindRenderbuffer(IRenderbuffer* buffer, size_t offset) override;
	virtual bool UnBindRenderbuffer(IRenderbuffer* buffer) override;
	virtual void* MapMemory(IRenderbuffer* buffer, size_t offset, size_t size) override;
	virtual void UnmapMemory(IRenderbuffer* buffer, size_t offset, size_t size) override;
	virtual bool FlushRenderbuffer(IRenderbuffer* buffer, size_t offset, size_t size) override;
	virtual bool ReadRenderbuffer(IRenderbuffer* buffer, size_t offset, size_t size, void** out_memory) override;
	virtual bool ResizeRenderbuffer(IRenderbuffer* buffer, size_t new_size) override;
	virtual bool LoadRange(IRenderbuffer* buffer, size_t offset, size_t size, const void* data) override;
	virtual bool CopyRange(IRenderbuffer* src, size_t src_offset, IRenderbuffer* dst, size_t dst_offset, size_t size) override;
	virtual bool DrawRenderbuffer(IRenderbuffer* buffer, size_t offset, uint32_t element_count, bool bind_only) override;
	virtual bool AllocateRenderbuffer(IRenderbuffer* buffer, size_t size, size_t* out_offset);
	virtual bool FreeRenderbuffer(IRenderbuffer* buffer, size_t size, size_t offset);

	// Render target
	virtual void SetViewport(Vector4 rect) override;
	virtual void ResetViewport() override;
	virtual void SetScissor(Vector4 rect) override;
	virtual void ResetScissor() override;

	// Shaders.
	virtual bool CreateShader(Shader* shader, const ShaderConfig* config, IRenderpass* pass, const std::vector<char*>& stage_filenames, std::vector<ShaderStage>& stages) override;
	virtual bool UseShader(Shader* shader) override;
	virtual bool BindGlobalsShader(Shader* shader) override;
	virtual bool BindInstanceShader(Shader* shader, uint32_t instance_id) override;
	virtual bool ApplyGlobalShader(Shader* shader) override;
	virtual bool ApplyInstanceShader(Shader* shader, bool need_update) override;
	virtual uint32_t AcquireInstanceResource(Shader* shader, std::vector<TextureMap*>& maps) override;
	virtual bool ReleaseInstanceResource(Shader* shader, uint32_t instance_id) override;
	virtual bool SetUniform(Shader* shader, ShaderUniform* uniform, const void* value) override;

	virtual bool AcquireTextureMap(TextureMap* map) override;
	virtual void ReleaseTextureMap(TextureMap* map) override;
	virtual bool GetEnabledMultiThread() const override;

public:
	virtual void CreateCommandBuffer();
	virtual bool RecreateSwapchain();

	virtual bool VerifyShaderID(uint32_t shader_id);

private:
	bool CreateModule(VulkanShader* shader);
	vk::SamplerAddressMode ConvertRepeatType(const char* axis, TextureRepeat repeat);
	vk::Filter ConvertFilterType(const char* op, TextureFilter filter);
	vk::Format ChannelCountToFormat(unsigned char channel_count, vk::Format default_format = vk::Format::eR8G8B8A8Unorm);

protected:
	VulkanContext Context;

};