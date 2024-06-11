#include "VulkanBuffer.hpp"

#include "VulkanContext.hpp"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

bool VulkanBuffer::Create(VulkanContext* context, size_t size, vk::BufferUsageFlags usage,
	vk::MemoryPropertyFlags memory_property_flags, bool bind_on_create, bool use_freelist) {

	TotalSize = size;
	Usage = usage;
	UseFreelist = use_freelist;
	MemoryPropertyFlags = memory_property_flags;

	// Create a new freelist.
	if (UseFreelist) {
		BufferFreelist.Create(size);
	}

	vk::BufferCreateInfo BufferInfo;
	BufferInfo.setSize(size)
		.setUsage(usage)
		.setSharingMode(vk::SharingMode::eExclusive);

	Buffer = context->Device.GetLogicalDevice().createBuffer(BufferInfo, context->Allocator);
	ASSERT(Buffer);

	// Gather memory requirements
	MemoryRequirements = context->Device.GetLogicalDevice().getBufferMemoryRequirements(Buffer);
	MemoryIndex = context->FindMemoryIndex(MemoryRequirements.memoryTypeBits, MemoryPropertyFlags);
	if (MemoryIndex == -1) {
		LOG_ERROR("Unable to create vulkan buffer because the required memory type index was not found.");

		CleanupFreelist();
		return false;
	}

	// Allocate memory info
	vk::MemoryAllocateInfo AllocateInfo;
	AllocateInfo.setAllocationSize(MemoryRequirements.size)
		.setMemoryTypeIndex((uint32_t)MemoryIndex);

	Memory = context->Device.GetLogicalDevice().allocateMemory(AllocateInfo, context->Allocator);
	ASSERT(Memory);

	// Determine if memory is on a device heap.
	bool IsDeviceMemory = (MemoryPropertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) == vk::MemoryPropertyFlagBits::eDeviceLocal;
	// Report memory as in-use
	Memory::AllocateReport(MemoryRequirements.size, IsDeviceMemory ? MemoryType::eMemory_Type_GPU_Local : MemoryType::eMemory_Type_Vulkan);

	if (bind_on_create) {
		Bind(context, Buffer, Memory, 0);
	}

	return true;
}

void VulkanBuffer::Destroy(VulkanContext* context) {
	if (UseFreelist) {
		CleanupFreelist();
	}

	if (Memory) {
		context->Device.GetLogicalDevice().freeMemory(Memory, context->Allocator);
		Memory = nullptr;
	}

	if (Buffer) {
		context->Device.GetLogicalDevice().destroyBuffer(Buffer, context->Allocator);
		Buffer = nullptr;
	}

	// Report the free memory.
	bool IsDeviceMemory = (MemoryPropertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) == vk::MemoryPropertyFlagBits::eDeviceLocal;
	Memory::FreeReport(MemoryRequirements.size, IsDeviceMemory ? MemoryType::eMemory_Type_GPU_Local : MemoryType::eMemory_Type_Vulkan);
	Memory::Zero(&MemoryRequirements, sizeof(vk::MemoryRequirements));

	TotalSize = 0;
	IsLocked = false;
}

bool VulkanBuffer::Resize(VulkanContext* context, size_t size, vk::Queue queue, vk::CommandPool pool) {

	// Sanity check
	if (size < TotalSize) {
		LOG_ERROR("Vulkan buffer resize requires that new size larger than the old. Nothing will doing.");
		return false;
	}

	// Resize the freelist first.
	if (UseFreelist) {
		if (!BufferFreelist.Resize(size)) {
			LOG_ERROR("Vulkan buffer resize failed to resize freelist.");
			return false;
		}
	}

	TotalSize = size;

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

	// Report the free of memory.
	bool IsDeviceMemory = (MemoryPropertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) == vk::MemoryPropertyFlagBits::eDeviceLocal;
	Memory::FreeReport(MemoryRequirements.size, IsDeviceMemory ? MemoryType::eMemory_Type_GPU_Local : MemoryType::eMemory_Type_Vulkan);
	MemoryRequirements = MemRequirements;
	Memory::AllocateReport(MemoryRequirements.size, IsDeviceMemory ? MemoryType::eMemory_Type_GPU_Local : MemoryType::eMemory_Type_Vulkan);

	// Set new properties
	TotalSize = size;
	Memory = NewMemory;
	Buffer = NewBuffer;

	return true;
}

void* VulkanBuffer::LockMemory(VulkanContext* context, vk::DeviceSize offset, vk::DeviceSize size, vk::MemoryMapFlags flags) {
	void* data = context->Device.GetLogicalDevice().mapMemory(Memory, offset, size, flags);
	return data;
}

void VulkanBuffer::UnlockMemory(VulkanContext* context) {
	context->Device.GetLogicalDevice().unmapMemory(Memory);
}

bool VulkanBuffer::Allocate(size_t size, size_t* offset) {
	if (offset == nullptr) {
		LOG_ERROR("Vulkan buffer allocate requires a valid point offset.");
		return false;
	}

	if (!UseFreelist) {
		LOG_WARN("Vulkan buffer allocate called on a buffer not using freelists. Offset will not be valid. Call vulkan_buffer_load_data instead.");
		*offset = 0;
		return true;
	}

	return BufferFreelist.AllocateBlock(size, offset);
}

bool VulkanBuffer::Free(size_t size, size_t offset) {
	if (size == 0) {
		LOG_ERROR("Vulkan buffer free requires a non-zero size.");
		return false;
	}

	if (!UseFreelist) {
		LOG_WARN("vulkan_buffer_free called on a buffer not using freelists. Nothing was done.");
		return true;
	}

	return BufferFreelist.FreeBlock(size, offset);
}

void VulkanBuffer::LoadData(VulkanContext* context, size_t offset, size_t size, vk::MemoryMapFlags flags, const void* data) {
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