#include "VulkanBuffer.hpp"

#include "VulkanContext.hpp"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"
#include "../Renderer.hpp"
#include "VulkanBackend.hpp"

VulkanBuffer::VulkanBuffer() {
	IRenderer* Renderer = IRenderer::GetRenderer();
	if (!Renderer) {
		return;
	}

	VulkanBackend* Backend = Cast<VulkanBackend*>(Renderer->GetRenderBackend());
	if (!Backend) {
		return;
	}

	Context = &Backend->Context;
}

VulkanBuffer::VulkanBuffer(EGPUBufferType type, size_t total_size, bool use_freelist) : VulkanBuffer() {
	TotalSize = total_size;
	Type = type;
	UseFreelist = use_freelist;

	if (!Create()) {
		GLOG(Log::eFatal, "Unable to create backing buffer for renderbuffer, Application cannot continue.");
		return;
	}
}

bool VulkanBuffer::Create() {
	if (!Context) {
		GLOG(Log::eError, "VulkanBuffer::Create() Context is null.");
		return false;
	}

	// Create freelist if needed.
	if (UseFreelist) {
		BufferFreelist.Create(TotalSize);
	}

	switch (Type)
	{
	case EGPUBufferType::eRenderbuffer_Type_Vertex:
		Usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc;
		MemoryPropertyFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;
		break;
	case EGPUBufferType::eRenderbuffer_Type_Index:
		Usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc;
		MemoryPropertyFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;
		break;
	case EGPUBufferType::eRenderbuffer_Type_Uniform:
	{
		vk::MemoryPropertyFlags DeviceLocalBits = Context->Device.GetIsSupportDeviceLocalHostVisible() ? vk::MemoryPropertyFlagBits::eDeviceLocal : vk::MemoryPropertyFlags(0);
		Usage = vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst;
		MemoryPropertyFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent | DeviceLocalBits;
	} break;
	case EGPUBufferType::eRenderbuffer_Type_Staging:
		Usage = vk::BufferUsageFlagBits::eTransferSrc;
		MemoryPropertyFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
		break;
	case EGPUBufferType::eRenderbuffer_Type_Read:
		Usage = vk::BufferUsageFlagBits::eTransferDst;
		MemoryPropertyFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
		break;
	case EGPUBufferType::eRenderbuffer_Type_Storage:
		GLOG(Log::eError, "Storage buffer not supported yet.");
		return false;
	default:
		GLOG(Log::eError, "Unsupported buffer type: %i.", Type);
		return false;
	}

	vk::BufferCreateInfo BufferInfo;
	BufferInfo.setSize(TotalSize)
		.setUsage(Usage)
		.setSharingMode(vk::SharingMode::eExclusive);

	Buffer = Context->Device.GetLogicalDevice().createBuffer(BufferInfo, Context->Allocator);
	ASSERT(Buffer);

	// Gather memory requirement.
	MemoryRequirements = Context->Device.GetLogicalDevice().getBufferMemoryRequirements(Buffer);
	MemoryIndex = Context->FindMemoryIndex(MemoryRequirements.memoryTypeBits, MemoryPropertyFlags);
	if (MemoryIndex == INVALID_ID) {
		GLOG(Log::eError, "Unable to create vulkan buffer because the required memory type index was not found.");
		return false;
	}

	// Allocate memory info.
	vk::MemoryAllocateInfo AllocateInfo;
	AllocateInfo.setAllocationSize(MemoryRequirements.size)
		.setMemoryTypeIndex(MemoryIndex);

	Memory = Context->Device.GetLogicalDevice().allocateMemory(AllocateInfo, Context->Allocator);
	ASSERT(Memory);

	// Determine if memory is on a device heap.
	bool IsDeviceMemory = (MemoryPropertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) == vk::MemoryPropertyFlagBits::eDeviceLocal;
	// Report memory as in-use
	Memory::AllocateReport(MemoryRequirements.size, IsDeviceMemory ? MemoryType::eMemory_Type_GPU_Local : MemoryType::eMemory_Type_Vulkan);

	return true;
}

void VulkanBuffer::Destroy() {
	if (!Context) {
		GLOG(Log::eError, "VulkanBuffer::Destroy() Context is null.");
		return;
	}

	if (UseFreelist) {
		BufferFreelist.Destroy();
	}

	if (Memory) {
		Context->Device.GetLogicalDevice().freeMemory(Memory, Context->Allocator);
		Memory = nullptr;
	}

	if (Buffer) {
		Context->Device.GetLogicalDevice().destroyBuffer(Buffer, Context->Allocator);
		Buffer = nullptr;
	}

	// Report the free memory.
	bool IsDeviceMemory = (MemoryPropertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) == vk::MemoryPropertyFlagBits::eDeviceLocal;
	Memory::FreeReport(MemoryRequirements.size, IsDeviceMemory ? MemoryType::eMemory_Type_GPU_Local : MemoryType::eMemory_Type_Vulkan);
	Memory::Zero(&MemoryRequirements, sizeof(vk::MemoryRequirements));

	TotalSize = 0;
	IsLocked = false;
}

bool VulkanBuffer::Resize(size_t new_size) {
	if (!Context) {
		GLOG(Log::eError, "VulkanBuffer::Resize() Context is null.");
		return false;
	}

	// Sanity check.
	if (new_size < TotalSize) {
		GLOG(Log::eError, "IRenderer::ResizeRenderbuffer() Failed to resize renderbuffer. Can not resize a smaller buffer.");
		return false;
	}

	if (UseFreelist) {
		// Resize the freelist first if used.
		if (!BufferFreelist.Resize(new_size)) {
			GLOG(Log::eError, "Vulkan buffer resize failed to resize freelist.");
			return false;
		}
	}

	TotalSize = new_size;

	// Create new buffer
	vk::BufferCreateInfo BufferInfo;
	BufferInfo.setSize(new_size)
		.setUsage(Usage)
		.setSharingMode(vk::SharingMode::eExclusive);

	vk::Buffer NewBuffer;
	NewBuffer = Context->Device.GetLogicalDevice().createBuffer(BufferInfo, Context->Allocator);
	ASSERT(NewBuffer);

	// Gather memory requirements
	vk::MemoryRequirements MemRequirements;
	MemRequirements = Context->Device.GetLogicalDevice().getBufferMemoryRequirements(NewBuffer);

	// Allocate memory info
	vk::MemoryAllocateInfo AllocateInfo;
	AllocateInfo.setAllocationSize(MemRequirements.size)
		.setMemoryTypeIndex((uint32_t)MemoryIndex);

	vk::DeviceMemory NewMemory;
	NewMemory = Context->Device.GetLogicalDevice().allocateMemory(AllocateInfo, Context->Allocator);
	ASSERT(NewMemory);

	// Bind the new buffer's memory
	Context->Device.GetLogicalDevice().bindBufferMemory(NewBuffer, NewMemory, 0);

	// Copy over the data
	//CopyRange(context, Buffer, 0, NewBuffer, 0, TotalSize);
	Context->Device.GetLogicalDevice().waitIdle();

	// Destroy the old
	if (Memory) {
		Context->Device.GetLogicalDevice().freeMemory(Memory, Context->Allocator);
		Memory = nullptr;
	}

	if (Buffer) {
		Context->Device.GetLogicalDevice().destroyBuffer(Buffer, Context->Allocator);
		Buffer = nullptr;
	}

	// Report the free of memory.
	bool IsDeviceMemory = (MemoryPropertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) == vk::MemoryPropertyFlagBits::eDeviceLocal;
	Memory::FreeReport(MemoryRequirements.size, IsDeviceMemory ? MemoryType::eMemory_Type_GPU_Local : MemoryType::eMemory_Type_Vulkan);
	MemoryRequirements = MemRequirements;
	Memory::AllocateReport(MemoryRequirements.size, IsDeviceMemory ? MemoryType::eMemory_Type_GPU_Local : MemoryType::eMemory_Type_Vulkan);

	// Set new properties
	Memory = NewMemory;
	Buffer = NewBuffer;

	return true;
}

bool VulkanBuffer::Bind(size_t offset) {
	if (!Context) {
		GLOG(Log::eError, "VulkanBuffer::Bind() Context is null.");
		return false;
	}

	Context->Device.GetLogicalDevice().bindBufferMemory(Buffer, Memory, offset);
	return true;
}

bool VulkanBuffer::UnBind() {
	// Vulkan 中 Buffer 与 Memory 的绑定是永久的，没有解绑操作。
	// 真正的资源释放在 Destroy() 中完成。
	return true;
}

void* VulkanBuffer::MapMemory(size_t offset, size_t size) {
	if (!Context) {
		GLOG(Log::eError, "VulkanBuffer::MapMemory() Context is null.");
		return nullptr;
	}

	void* data = Context->Device.GetLogicalDevice().mapMemory(Memory, offset, size, vk::MemoryMapFlags());
	return data;
}

void VulkanBuffer::UnmapMemory() {
	if (!Context) {
		GLOG(Log::eError, "VulkanBuffer::UnmapMemory() Context is null.");
		return;
	}

	Context->Device.GetLogicalDevice().unmapMemory(Memory);
}

bool VulkanBuffer::Flush(size_t offset, size_t size){
	// NOTE: If not host-coherent, flush the mapped memory range.
	if (!IsHostCoherent()) {
		if (!Context) {
			GLOG(Log::eError, "VulkanBuffer::Flush() Context is null.");
			return false;
		}

		vk::MappedMemoryRange Range;
		Range.setMemory(Memory)
			.setOffset(offset)
			.setSize(size);

		if (Context->Device.GetLogicalDevice().flushMappedMemoryRanges(1, &Range) != vk::Result::eSuccess) {
			GLOG(Log::eError, "Flush mapped memory ranges failed.");
			return false;
		}
	}

	return true;
}

bool VulkanBuffer::CopyRange(IGPUBuffer* src, size_t src_offset, size_t dst_offset, size_t size) {
	VulkanBuffer* SrcBuffer = Cast<VulkanBuffer*>(src);
	if (!SrcBuffer) {
		GLOG(Log::eError, "Unable to copy buffer because the source buffer is not a vulkan buffer.");
		return false;
	}

	vk::Queue GraphsicQueue = Context->Device.GetGraphicsQueue();
	vk::CommandPool CmdPool = Context->Device.GetGraphicsCommandPool();
	GraphsicQueue.waitIdle();

	// Create a one-time-use command buffer
	VulkanCommandBuffer SingalCommandBuffer;
	SingalCommandBuffer.AllocateAndBeginSingleUse(Context, CmdPool);

	// Prepare the copy command and add it to the command buffer
	vk::BufferCopy CopyRegion;
	CopyRegion.setSrcOffset(src_offset)
		.setDstOffset(dst_offset)
		.setSize(size);

	SingalCommandBuffer.CommandBuffer.copyBuffer(SrcBuffer->Buffer, Buffer, CopyRegion);

	// Submit the buffer for execution and wait for it to complete
	SingalCommandBuffer.EndSingleUse(Context, CmdPool, GraphsicQueue);

	return true;
}

bool VulkanBuffer::AllocateMemory(size_t size, size_t* out_offset) {
	if (size == 0 || out_offset == nullptr) {
		GLOG(Log::eError, "IRenderer::AllocateRenderbuffer() Requires valid pointer, a non-zero size and valid pointer to hlod offset.");
		return false;
	}

	if (!UseFreelist) {
		GLOG(Log::eWarn, "IRenderer::AllocateRenderbuffer() Called on a buffer not using freelist. Offset will not be valid. Call LoadData() instead.");
		*out_offset = 0;
		return true;
	}

	return BufferFreelist.AllocateBlock(size, out_offset);
}

bool VulkanBuffer::FreeMemory(size_t size, size_t offset) {
	if (size == 0) {
		GLOG(Log::eError, "IRenderer::FreeRenderbuffer() Requires valid pointer, a non-zero size.");
		return false;
	}

	if (!UseFreelist) {
		GLOG(Log::eWarn, "IRenderer::FreeRenderbuffer() Called on a buffer not using freelist. Nothing was down.");
		return true;
	}

	return BufferFreelist.FreeBlock(size, offset);
}

bool VulkanBuffer::Load(size_t offset, size_t size, const void* data) {
	if (!Context) {
		GLOG(Log::eError, "VulkanBuffer::Load() Context is null.");
		return false;
	}

	if (IsDeviceLocal() && !IsHostVisible()) {
		// NOTE: If a staging buffer is needed (i.e.) the target buffer's memory is not host visible but is device-local,
		// create a staging buffer to load the data into first. Then copy from it to the target buffer.

		// Create a host-visible staging buffer to upload to. Mark it as the source of the transfer.
		VulkanBuffer Staging;
		Staging.Type = EGPUBufferType::eRenderbuffer_Type_Staging;
		Staging.TotalSize = size;
		Staging.UseFreelist = false;
		if (!Staging.Create()) {
			GLOG(Log::eError, "VulkanBackend::ReadRenderbuffer() Failed to create staging buffer.");
			return false;
		}

		Staging.Bind(0);

		// Load the data into the staging buffer.
		Staging.Load(0, size, data);

		// Perform the copy from staging to the device local buffer.
		CopyRange(&Staging, 0, offset, size);

		// Clean up the staging buffer.
		Staging.UnBind();
	}
	else {
		// If no staging buffer is needed, map/copy/unmap.
		void* DataPtr;
		if (Context->Device.GetLogicalDevice().mapMemory(Memory, offset, size, vk::MemoryMapFlags(), &DataPtr) != vk::Result::eSuccess) {
			GLOG(Log::eError, "Map memory Failed.");
			return false;
		}
		Memory::Copy(DataPtr, data, size);
		Context->Device.GetLogicalDevice().unmapMemory(Memory);
	}

	return true;
}

TArray<uint8_t> VulkanBuffer::Read(size_t offset, size_t size) {
	TArray<uint8_t> Result(size);

	if (IsDeviceLocal() && !IsHostVisible()) {
		VulkanBuffer ReadBuf;
		ReadBuf.Type = EGPUBufferType::eRenderbuffer_Type_Read;
		ReadBuf.TotalSize = size;
		ReadBuf.UseFreelist = false;
		ReadBuf.Create();
		ReadBuf.Bind(0);

		ReadBuf.CopyRange(this, offset, 0, size);

		void* MappedData = ReadBuf.MapMemory(0, size);
		Memory::Copy(Result.Data(), MappedData, size);
		ReadBuf.UnmapMemory();
		ReadBuf.Destroy();
	}
	else {
		void* MappedData = MapMemory(offset, size);
		Memory::Copy(Result.Data(), MappedData, size);
		UnmapMemory();
	}

	return Result;
}