#pragma once

#include <vulkan/vulkan.hpp>
#include "Memory/Freelist.hpp"
#include "Rendering/Interface/IRenderbuffer.hpp"

class VulkanContext;
enum class EGPUBufferType;

class VulkanBuffer : public IGPUBuffer {
public:
	VulkanBuffer();
	VulkanBuffer(EGPUBufferType type, size_t total_size, bool use_freelist);
	virtual ~VulkanBuffer() { Destroy(); }

public:
	virtual bool Create() override;
	virtual void Destroy() override;
	virtual bool Resize(size_t new_size) override;
	virtual bool Bind(size_t offset) override;
	virtual bool UnBind() override;
	virtual void* MapMemory(size_t offset, size_t size) override;
	virtual void UnmapMemory() override;
	virtual bool Flush(size_t offset, size_t size) override;
	virtual bool CopyRange(IGPUBuffer* src, size_t src_offset, size_t dst_offset, size_t size) override;
	virtual bool AllocateMemory(size_t size, size_t* out_offset) override;
	virtual bool FreeMemory(size_t size, size_t offset) override;
	virtual bool Load(size_t offset, size_t size, const void* data) override;
	virtual TArray<uint8_t> Read(size_t offset, size_t size) override;

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
	VulkanContext* Context;
	vk::Buffer Buffer;
	vk::BufferUsageFlags Usage;
	bool IsLocked = false;
	vk::DeviceMemory Memory;
	vk::MemoryRequirements MemoryRequirements;
	int MemoryIndex = -1;
	vk::MemoryPropertyFlags MemoryPropertyFlags;
};