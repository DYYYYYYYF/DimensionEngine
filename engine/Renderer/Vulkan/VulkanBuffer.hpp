#pragma once

#include <vulkan/vulkan.hpp>

class VulkanContext;

class VulkanBuffer {
public:
	VulkanBuffer() {}
	virtual ~VulkanBuffer() {}

public:
	bool Create(VulkanContext* context, size_t size, vk::BufferUsageFlags usage,
		vk::MemoryPropertyFlags memory_property_flags, bool bind_on_create);
	void Destroy(VulkanContext* context);

	bool Resize(VulkanContext* context, size_t size, vk::Queue queue, vk::CommandPool pool);

	void* LockMemory(VulkanContext* context, vk::DeviceSize offset, vk::DeviceSize size, vk::MemoryMapFlags flags);
	void UnlockMemory(VulkanContext* context);

	void LoadData(VulkanContext* context, size_t offset, size_t size, vk::MemoryMapFlags flags, const void* data);

	void Bind(VulkanContext* context, vk::Buffer buf, vk::DeviceMemory mem, size_t offset);
	void CopyTo(VulkanContext* context, vk::CommandPool pool, vk::Fence fence, vk::Queue queue, vk::Buffer src, size_t src_offset,
		vk::Buffer dst, size_t dst_offset, size_t size);

public:
	size_t TotalSize;
	vk::Buffer Buffer;
	vk::BufferUsageFlags Usage;
	bool IsLocked;
	vk::DeviceMemory Memory;
	int MemoryIndex;
	vk::MemoryPropertyFlags MemoryPropertyFlags;
};