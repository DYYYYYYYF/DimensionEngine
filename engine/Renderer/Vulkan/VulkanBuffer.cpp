#include "VulkanBuffer.hpp"

#include "VulkanContext.hpp"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

bool VulkanBuffer::Create(VulkanContext* context, size_t size, vk::BufferUsageFlags usage,
	vk::MemoryPropertyFlags memory_property_flags, bool bind_on_create) {

	TotalSize = size;
	Usage = usage;
	MemoryPropertyFlags = memory_property_flags;

	vk::BufferCreateInfo BufferInfo;
	BufferInfo.setSize(size)
		.setUsage(usage)
		.setSharingMode(vk::SharingMode::eExclusive);

	Buffer = context->Device.GetLogicalDevice().createBuffer(BufferInfo, context->Allocator);
	ASSERT(Buffer);

	// Gather memory requirements
	vk::MemoryRequirements MemRequirements;
	MemRequirements = context->Device.GetLogicalDevice().getBufferMemoryRequirements(Buffer);
	MemoryIndex = context->FindMemoryIndex(MemRequirements.memoryTypeBits, MemoryPropertyFlags);
	if (MemoryIndex == -1) {
		UL_ERROR("Unable to create vulkan buffer because the required memory type index was not found.");
		return false;
	}

	// Allocate memory info
	vk::MemoryAllocateInfo AllocateInfo;
	AllocateInfo.setAllocationSize(MemRequirements.size)
		.setMemoryTypeIndex((uint32_t)MemoryIndex);

	Memory = context->Device.GetLogicalDevice().allocateMemory(AllocateInfo, context->Allocator);
	ASSERT(Memory);

	if (bind_on_create) {
		Bind(context, Buffer, Memory, 0);
	}

	return true;
}

void VulkanBuffer::Destroy(VulkanContext* context) {
	if (Memory) {
		context->Device.GetLogicalDevice().freeMemory(Memory, context->Allocator);
		Memory = nullptr;
	}

	if (Buffer) {
		context->Device.GetLogicalDevice().destroyBuffer(Buffer, context->Allocator);
		Buffer = nullptr;
	}

	TotalSize = 0;
	IsLocked = false;
}

bool VulkanBuffer::Resize(VulkanContext* context, size_t size, vk::Queue queue, vk::CommandPool pool) {

	// Create new buffer
	vk::BufferCreateInfo BufferInfo;
	BufferInfo.setSize(size)
		.setUsage(Usage)
		.setSharingMode(vk::SharingMode::eExclusive);

	vk::Buffer NewBuffer;
	NewBuffer = context->Device.GetLogicalDevice().createBuffer(BufferInfo, context->Allocator);
	ASSERT(NewBuffer);

	// Gather memory requirements
	vk::MemoryRequirements MemRequirements;
	MemRequirements = context->Device.GetLogicalDevice().getBufferMemoryRequirements(Buffer);

	// Allocate memory info
	vk::MemoryAllocateInfo AllocateInfo;
	AllocateInfo.setAllocationSize(MemRequirements.size)
		.setMemoryTypeIndex((uint32_t)MemoryIndex);

	vk::DeviceMemory NewMemory;
	NewMemory = context->Device.GetLogicalDevice().allocateMemory(AllocateInfo, context->Allocator);
	ASSERT(NewMemory);

	// Bind the new buffer's memory
	context->Device.GetLogicalDevice().bindBufferMemory(NewBuffer, NewMemory, 0);

	// Copy over the data
	CopyTo(context, pool, nullptr, queue, Buffer, 0, NewBuffer, 0, TotalSize);
	context->Device.GetLogicalDevice().waitIdle();

	// Destroy the old
	if (Memory) {
		context->Device.GetLogicalDevice().freeMemory(Memory, context->Allocator);
		Memory = nullptr;
	}

	if (Buffer) {
		context->Device.GetLogicalDevice().destroyBuffer(Buffer, context->Allocator);
		Buffer = nullptr;
	}

	// Set new properties
	TotalSize = size;
	Memory = NewMemory;
	Buffer = NewBuffer;

	return true;
}

void* VulkanBuffer::LockMemory(VulkanContext* context, vk::DeviceSize offset, vk::DeviceSize size, vk::MemoryMapFlagBits flags) {
	void* data = context->Device.GetLogicalDevice().mapMemory(Memory, offset, size, flags);
	return data;
}

void VulkanBuffer::UnlockMemory(VulkanContext* context) {
	context->Device.GetLogicalDevice().unmapMemory(Memory);
}

void VulkanBuffer::LoadData(VulkanContext* context, size_t offset, size_t size, vk::MemoryMapFlagBits flags, const void* data) {
	void* DataPtr = context->Device.GetLogicalDevice().mapMemory(Memory, offset, size, flags);
	Memory::Copy(DataPtr, data, size);
	context->Device.GetLogicalDevice().unmapMemory(Memory);
}

void VulkanBuffer::Bind(VulkanContext* context, vk::Buffer buf, vk::DeviceMemory mem, size_t offset) {
	context->Device.GetLogicalDevice().bindBufferMemory(buf, mem, offset);
}

void VulkanBuffer::CopyTo(VulkanContext* context, vk::CommandPool pool, vk::Fence fence, vk::Queue queue, vk::Buffer src, size_t src_offset,
	vk::Buffer dst, size_t dst_offset, size_t size) {
	queue.waitIdle();

	// Create a one-time-use command buffer
	VulkanCommandBuffer SingalCommandBuffer;
	SingalCommandBuffer.AllocateAndBeginSingleUse(context, pool);

	// Prepare the copy command and add it to the command buffer
	vk::BufferCopy CopyRegion;
	CopyRegion.setSrcOffset(src_offset)
		.setDstOffset(dst_offset)
		.setSize(size);

	SingalCommandBuffer.CommandBuffer.copyBuffer(src, dst, CopyRegion);

	// Submit the buffer for execution and wait for it to complete
	SingalCommandBuffer.EndSingleUse(context, pool, queue);
}