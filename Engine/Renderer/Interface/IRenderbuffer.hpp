#pragma once

#include <vulkan/vulkan.hpp>

class VulkanContext;

enum RenderbufferType {
	eRenderbuffer_Type_Unknown,		// Default.
	eRenderbuffer_Type_Vertex,		// Buffer is used for vertex data.
	eRenderbuffer_Type_Index,		// Buffer is used for index data.
	eRenderbuffer_Type_Uniform,		// Buffer is used for uniform data.
	eRenderbuffer_Type_Staging,		// Buffer is used for staging purposes (i.e. from host-visible to device-local memory)
	eRenderbuffer_Type_Read,		// Buffer is used for staging purposes (i.e. Copy to from device local. then read)
	eRenderbuffer_Type_Storage		// Buffer is used for data storage.
};

class IRenderbuffer {
public:
	virtual bool Create(VulkanContext* context) = 0;
	virtual void Destroy(VulkanContext* context) = 0;
	virtual bool Bind(VulkanContext* context, size_t offset) = 0;
	virtual bool UnBind(VulkanContext* context) = 0;
	virtual void* MapMemory(VulkanContext* context, size_t offset, size_t size) = 0;
	virtual void UnmapMemory(VulkanContext* context) = 0;
	virtual bool Flush(VulkanContext* context, size_t offset, size_t size) = 0;
	virtual bool Resize(VulkanContext* context, size_t new_size) = 0;
	virtual bool CopyRange(VulkanContext* context, vk::Buffer src, size_t src_offset, vk::Buffer dst, size_t dst_offset, size_t size) = 0;

	virtual bool IsDeviceLocal() { return false; };
	virtual bool IsHostVisible() { return false; };
	virtual bool IsHostCoherent() { return false; };

public:
	RenderbufferType Type;
	size_t TotalSize;
	Freelist BufferFreelist;
	bool UseFreelist;
};
