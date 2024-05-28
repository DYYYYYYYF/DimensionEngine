#pragma once

#include "Renderer/Interface/IRendererBackend.hpp"
#include "Core/DMemory.hpp"
#include "VulkanContext.hpp"

class VulkanBuffer;

class VulkanBackend : public IRendererBackend {
public:
	VulkanBackend();
	virtual ~VulkanBackend();

public:
	virtual bool Initialize(const RenderBackendConfig* config, unsigned char* out_window_render_target_count, struct SPlatformState* plat_state) override;
	virtual void Shutdown() override;

	virtual bool BeginFrame(double delta_time) override;
	virtual void DrawGeometry(GeometryRenderData geometry) override;
	virtual bool EndFrame(double delta_time) override;
	virtual void Resize(unsigned short width, unsigned short height) override;

	// Textures
	virtual void CreateTexture(const unsigned char* pixels, Texture* texture) override;
	virtual void DestroyTexture(Texture* texture) override;
	virtual void CreateWriteableTexture(Texture* tex) override;
	virtual void ResizeTexture(Texture* tex, uint32_t new_width, uint32_t new_height) override;
	virtual void WriteTextureData(Texture* tex, uint32_t offset, uint32_t size, const unsigned char* pixels) override;

	virtual bool CreateGeometry(Geometry* geometry, uint32_t vertex_size, uint32_t vertex_count, 
		const void* vertices, uint32_t index_size, uint32_t index_count, const void* indices) override;
	virtual void DestroyGeometry(Geometry* geometry) override;

	// Renderpass
	virtual bool BeginRenderpass(IRenderpass* pass, RenderTarget* target) override;
	virtual bool EndRenderpass(IRenderpass* pass) override;
	virtual IRenderpass* GetRenderpass(const char* name) override;
	virtual void CreateRenderTarget(unsigned char attachment_count, std::vector<Texture*> attachments, IRenderpass* pass, uint32_t width, uint32_t height, RenderTarget* out_target) override;
	virtual void DestroyRenderTarget(RenderTarget* target, bool free_internal_memory) override;
	virtual Texture* GetWindowAttachment(unsigned char index) override;
	virtual Texture* GetDepthAttachment() override;
	virtual unsigned char GetWindowAttachmentIndex() override;
	virtual void CreateRenderpass(IRenderpass* out_renderpass, float depth, uint32_t stencil, bool has_prev_pass, bool has_next_pass) override;
	virtual void DestroyRenderpass(IRenderpass* pass) override;

	// Shaders.
	virtual bool CreateShader(Shader* shader, IRenderpass* pass, unsigned short stage_count, const std::vector<char*>& stage_filenames, std::vector<ShaderStage>& stages) override;
	virtual bool DestroyShader(Shader* shader) override;
	virtual bool InitializeShader(Shader* shader) override;
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

public:
	virtual bool CreateBuffers();
	virtual bool UploadDataRange(vk::CommandPool pool, vk::Fence fence, vk::Queue queue, VulkanBuffer* buffer, size_t* offset, size_t size, const void* data);
	virtual void FreeDataRange(VulkanBuffer* buffer, size_t offset, size_t size);

	virtual void CreateCommandBuffer();
	virtual bool RecreateSwapchain();

	virtual bool VerifyShaderID(uint32_t shader_id);

private:
	bool CreateModule(VulkanShader* shader, VulkanShaderStageConfig config, VulkanShaderStage* shader_stage);
	vk::SamplerAddressMode ConvertRepeatType(const char* axis, TextureRepeat repeat);
	vk::Filter ConvertFilterType(const char* op, TextureFilter filter);
	vk::Format ChannelCountToFormat(unsigned char channel_count, vk::Format default_format = vk::Format::eR8G8B8A8Unorm);

protected:
	VulkanContext Context;

};