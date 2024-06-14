#include "VulkanBuffer.hpp"

#include "VulkanContext.hpp"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

bool VulkanBuffer::Create(VulkanContext* context) {
	switch (Type)
	{
	case eRenderbuffer_Type_Vertex:
		Usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc;
		MemoryPropertyFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;
		break;
	case eRenderbuffer_Type_Index:
		Usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc;
		MemoryPropertyFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;
		break;
	case eRenderbuffer_Type_Uniform:
	{
		vk::MemoryPropertyFlags DeviceLocalBits = context->Device.GetIsSupportDeviceLocalHostVisible() ? vk::MemoryPropertyFlagBits::eDeviceLocal : vk::MemoryPropertyFlags(0);
		Usage = vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst;
		MemoryPropertyFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent | DeviceLocalBits;
	} break;
	case eRenderbuffer_Type_Staging:
		Usage = vk::BufferUsageFlagBits::eTransferSrc;
		MemoryPropertyFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
		break;
	case eRenderbuffer_Type_Read:
		Usage = vk::BufferUsageFlagBits::eTransferDst;
		MemoryPropertyFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
		break;
	case eRenderbuffer_Type_Storage:
		LOG_ERROR("Storage buffer not supported yet.");
		return false;
	default:
		LOG_ERROR("Unsupported buffer type: %i.", Type);
		return false;
	}

	vk::BufferCreateInfo BufferInfo;
	BufferInfo.setSize(TotalSize)
		.setUsage(Usage)
		.setSharingMode(vk::SharingMode::eExclusive);

	Buffer = context->Device.GetLogicalDevice().createBuffer(BufferInfo, context->Allocator);
	ASSERT(Buffer);

	// Gather memory requirement.
	MemoryRequirements = context->Device.GetLogicalDevice().getBufferMemoryRequirements(Buffer);
	MemoryIndex = context->FindMemoryIndex(MemoryRequirements.memoryTypeBits, MemoryPropertyFlags);
	if (MemoryIndex == -1) {
		LOG_ERROR("Unable to create vulkan buffer because the required memory type index was not found.");
		return false;
	}

	// Allocate memory info.
	vk::MemoryAllocateInfo AllocateInfo;
	AllocateInfo.setAllocationSize(MemoryRequirements.size)
		.setMemoryTypeIndex(MemoryIndex);

	Memory = context->Device.GetLogicalDevice().allocateMemory(AllocateInfo, context->Allocator);
	ASSERT(Memory);

	// Determine if memory is on a device heap.
	bool IsDeviceMemory = (MemoryPropertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) == vk::MemoryPropertyFlagBits::eDeviceLocal;
	// Report memory as in-use
	Memory::AllocateReport(MemoryRequirements.size, IsDeviceMemory ? MemoryType::eMemory_Type_GPU_Local : MemoryType::eMemory_Type_Vulkan);

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

	// Report the free memory.
	bool IsDeviceMemory = (MemoryPropertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) == vk::MemoryPropertyFlagBits::eDeviceLocal;
	Memory::FreeReport(MemoryRequirements.size, IsDeviceMemory ? MemoryType::eMemory_Type_GPU_Local : MemoryType::eMemory_Type_Vulkan);
	Memory::Zero(&MemoryRequirements, sizeof(vk::MemoryRequirements));

	TotalSize = 0;
	IsLocked = false;
}

bool VulkanBuffer::Resize(VulkanContext* context, size_t size) {

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
	CopyRange(context, Buffer, 0, NewBuffer, 0, TotalSize);
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

bool VulkanBuffer::Bind(VulkanContext* context, size_t offset) {
	context->Device.GetLogicalDevice().bindBufferMemory(Buffer, Memory, offset);
	return true;
}

bool VulkanBuffer::UnBind(VulkanContext* context) {
	return true;
}

void* VulkanBuffer::MapMemory(VulkanContext* context, size_t offset, size_t size) {
	void* data = context->Device.GetLogicalDevice().mapMemory(Memory, offset, size, vk::MemoryMapFlags());
	return data;
}

void VulkanBuffer::UnmapMemory(VulkanContext* context) {
	context->Device.GetLogicalDevice().unmapMemory(Memory);
}

bool VulkanBuffer::Flush(VulkanContext* context, size_t offset, size_t size){
	// NOTE: If not host-coherent, flush the mapped memory range.
	if (!IsHostCoherent()) {
		vk::MappedMemoryRange Range;
		Range.setMemory(Memory)
			.setOffset(offset)
			.setSize(size);

		if (context->Device.GetLogicalDevice().flushMappedMemoryRanges(1, &Range) != vk::Result::eSuccess) {
			LOG_ERROR("Flush mapped memory ranges failed.");
			return false;
		}
	}

	return true;
}

bool VulkanBuffer::CopyRange(VulkanContext* context, vk::Buffer src, size_t src_offset, vk::Buffer dst, size_t dst_offset, size_t size) {
	vk::Queue GraphsicQueue = context->Device.GetGraphicsQueue();
	vk::CommandPool CmdPool = context->Device.GetGraphicsCommandPool();
	GraphsicQueue.waitIdle();

	// Create a one-time-use command buffer
	VulkanCommandBuffer SingalCommandBuffer;
	SingalCommandBuffer.AllocateAndBeginSingleUse(context, CmdPool);

	// Prepare the copy command and add it to the command buffer
	vk::BufferCopy CopyRegion;
	CopyRegion.setSrcOffset(src_offset)
		.setDstOffset(dst_offset)
		.setSize(size);

	SingalCommandBuffer.CommandBuffer.copyBuffer(src, dst, CopyRegion);

	// Submit the buffer for execution and wait for it to complete
	SingalCommandBuffer.EndSingleUse(context, CmdPool, GraphsicQueue);

	return true;
}