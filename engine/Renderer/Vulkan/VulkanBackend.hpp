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
	virtual void UpdateGlobalWorldState(Matrix4 projection, Matrix4 view, Vec3 view_position, Vec4 ambient_color, int mode) override;
	virtual void UpdateGlobalUIState(Matrix4 projection, Matrix4 view, int mode) override;
	virtual void DrawGeometry(GeometryRenderData geometry) override;
	virtual bool EndFrame(double delta_time) override;
	virtual void Resize(unsigned short width, unsigned short height) override;

	virtual void CreateTexture(const unsigned char* pixels, Texture* texture) override;
	virtual void DestroyTexture(Texture* texture) override;

	virtual bool CreateMaterial(Material* material) override;
	virtual void DestroyMaterial(Material* material) override;

	virtual bool CreateGeometry(Geometry* geometry, uint32_t vertex_size, uint32_t vertex_count, 
		const void* vertices, uint32_t index_size, uint32_t index_count, const void* indices) override;
	virtual void DestroyGeometry(Geometry* geometry) override;

	virtual bool BeginRenderpass(unsigned char renderpass_id) override;
	virtual bool EndRenderpass(unsigned char renderpass_id) override;

public:
	virtual bool CreateBuffers();
	virtual void UploadDataRange(vk::CommandPool pool, vk::Fence fence, vk::Queue queue, VulkanBuffer* buffer, size_t offset, size_t size, const void* data);

	virtual void CreateCommandBuffer();
	virtual void RegenerateFrameBuffers();
	virtual bool RecreateSwapchain();

protected:
	VulkanContext Context;

};