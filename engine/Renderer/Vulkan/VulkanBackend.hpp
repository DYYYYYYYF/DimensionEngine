#pragma once

#include "Renderer/RendererBackend.hpp"
#include "Core/DMemory.hpp"
#include "VulkanContext.hpp"

class VulkanBuffer;

class VulkanBackend : public IRendererBackend {
public:
	VulkanBackend();
	virtual ~VulkanBackend();

public:
	virtual bool Initialize(const char* application_name, struct SPlatformState* plat_state) override;
	virtual void Shutdown() override;

	virtual bool BeginFrame(double delta_time) override;
	virtual void DrawGeometry(GeometryRenderData geometry) override;
	virtual bool EndFrame(double delta_time) override;
	virtual void Resize(unsigned short width, unsigned short height) override;

	virtual void CreateTexture(const unsigned char* pixels, Texture* texture) override;
	virtual void DestroyTexture(Texture* texture) override;

	virtual bool CreateGeometry(Geometry* geometry, uint32_t vertex_size, uint32_t vertex_count, 
		const void* vertices, uint32_t index_size, uint32_t index_count, const void* indices) override;
	virtual void DestroyGeometry(Geometry* geometry) override;

	virtual bool BeginRenderpass(unsigned char renderpass_id) override;
	virtual bool EndRenderpass(unsigned char renderpass_id) override;

public:
	// Shaders.
	virtual bool CreateShader(Shader* shader, unsigned short renderpass_id, unsigned short stage_count, const std::vector<char*>& stage_filenames, std::vector<ShaderStage>& stages) override;
	virtual bool DestroyShader(Shader* shader) override;
	virtual bool InitializeShader(Shader* shader) override;
	virtual bool UseShader(Shader* shader) override;
	virtual bool BindGlobalsShader(Shader* shader) override;
	virtual bool BindInstanceShader(Shader* shader, uint32_t instance_id) override;
	virtual bool ApplyGlobalShader(Shader* shader) override;
	virtual bool ApplyInstanceShader(Shader* shader) override;
	virtual uint32_t AcquireInstanceResource(Shader* shader) override;
	virtual bool ReleaseInstanceResource(Shader* shader, uint32_t instance_id) override;
	virtual bool SetUniform(Shader* shader, ShaderUniform* uniform, const void* value) override;

public:
	virtual bool CreateBuffers();
	virtual bool UploadDataRange(vk::CommandPool pool, vk::Fence fence, vk::Queue queue, VulkanBuffer* buffer, size_t* offset, size_t size, const void* data);
	virtual void FreeDataRange(VulkanBuffer* buffer, size_t offset, size_t size);

	virtual void CreateCommandBuffer();
	virtual void RegenerateFrameBuffers();
	virtual bool RecreateSwapchain();

	virtual bool VerifyShaderID(uint32_t shader_id);

private:
	bool CreateModule(VulkanShader* shader, VulkanShaderStageConfig config, VulkanShaderStage* shader_stage);

protected:
	VulkanContext Context;

};