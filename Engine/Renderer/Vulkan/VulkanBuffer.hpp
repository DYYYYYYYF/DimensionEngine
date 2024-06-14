#pragma once

#include <vulkan/vulkan.hpp>
#include "Containers/Freelist.hpp"
#include "Renderer/Interface/IRenderbuffer.hpp"

class VulkanContext;

class VulkanBuffer : public IRenderbuffer {
public:
	VulkanBuffer() {}
	virtual ~VulkanBuffer() {}

public:
	virtual bool Create(VulkanContext* context) override;
	virtual void Destroy(VulkanContext* context) override;
	virtual bool Resize(VulkanContext* context, size_t size) override;
	virtual bool Bind(VulkanContext* context, size_t offset) override;
	virtual bool UnBind(VulkanContext* context) override;
	virtual void* MapMemory(VulkanContext* context, size_t offset, size_t size) override;
	virtual void UnmapMemory(VulkanContext* context) override;
	virtual bool Flush(VulkanContext* context, size_t offset, size_t size) override;
	virtual bool CopyRange(VulkanContext* context, vk::Buffer src, size_t src_offset, vk::Buffer dst, size_t dst_offset, size_t size) override;

public:
	virtual bool IsDeviceLocal() override {
		return ((MemoryPropertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) == vk::MemoryPropertyFlagBits::eDeviceLocal);
	}

	virtual bool IsHostVisible() override {
		return ((MemoryPropertyFlags & vk::MemoryPropertyFlagBits::eHostVisible) == vk::MemoryPropertyFlagBits::eHostVisible);
	}

	virtual bool IsHostCoherent() override {
		return ((MemoryPropertyFlags & vk::MemoryPropertyFlagBits::eHostCoherent) == vk::MemoryPropertyFlagBits::eHostCoherent);
	}

public:
	vk::Buffer Buffer;
	vk::BufferUsageFlags Usage;
	bool IsLocked;
	vk::DeviceMemory Memory;
	vk::MemoryRequirements MemoryRequirements;
	int MemoryIndex;
	vk::MemoryPropertyFlags MemoryPropertyFlags;
};