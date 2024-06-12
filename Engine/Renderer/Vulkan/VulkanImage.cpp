#include "VulkanImage.hpp"

#include "VulkanContext.hpp"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

void VulkanImage::CreateImage(VulkanContext* context, TextureType type, uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
	vk::ImageUsageFlags usage, vk::MemoryPropertyFlags memory_flags, bool create_view, vk::ImageAspectFlags view_aspect_flags) {
	Width = width;
	Height = height;
	MemoryFlags = memory_flags;

	vk::Device LogicalDevice = context->Device.GetLogicalDevice();

	vk::Extent3D Extent;
	Extent.setWidth(Width)
		.setHeight(Height)
		.setDepth(1);			// TODO: Support configurable depth

	vk::ImageCreateInfo ImageCreateInfo;
	switch (type)
	{
	default:
	case eTexture_Type_2D:
	case eTexture_Type_Cube:
		ImageCreateInfo.setImageType(vk::ImageType::e2D);
		break;
	}

	ImageCreateInfo.setExtent(Extent)
		.setMipLevels(4)		// TODO: Support mip mapping
		.setArrayLayers(1)		
		.setFormat(format)
		.setTiling(tiling)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setUsage(usage)
		.setSamples(vk::SampleCountFlagBits::e1)		// TODO: Configurable sample count
		.setSharingMode(vk::SharingMode::eExclusive);	// TODO: Configurable sharing mode
	if (type == TextureType::eTexture_Type_Cube) {
		ImageCreateInfo.setFlags(vk::ImageCreateFlagBits::eCubeCompatible)
			.setArrayLayers(6);
	}

	Image = LogicalDevice.createImage(ImageCreateInfo, context->Allocator);
	ASSERT(Image);

	// Query memory requirements
	MemoryRequirements = LogicalDevice.getImageMemoryRequirements(Image);
	uint32_t MemoryType = context->FindMemoryIndex(MemoryRequirements.memoryTypeBits, MemoryFlags);
	if (MemoryType == -1) {
		LOG_ERROR("Required memory type not found. Image not vaild.");
	}

	// Allocate memory
	vk::MemoryAllocateInfo MemoryAllocateInfo;
	MemoryAllocateInfo.setAllocationSize(MemoryRequirements.size)
		.setMemoryTypeIndex(MemoryType);
	DeviceMemory = LogicalDevice.allocateMemory(MemoryAllocateInfo, context->Allocator);
	ASSERT(DeviceMemory);

	// Bind memory
	LogicalDevice.bindImageMemory(Image, DeviceMemory, 0);

	// Report the memory as in-use.
	bool IsDeviceMemory = (MemoryFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) == vk::MemoryPropertyFlagBits::eDeviceLocal;
	Memory::AllocateReport(MemoryRequirements.size, IsDeviceMemory ? MemoryType::eMemory_Type_GPU_Local : MemoryType::eMemory_Type_Vulkan);

	// Create image view
	if (create_view) {
		CreateImageView(context, type, format, view_aspect_flags);
	}

}

void VulkanImage::CreateImageView(VulkanContext* context, TextureType type, vk::Format format, vk::ImageAspectFlags view_aspect_flags) {
	vk::ImageSubresourceRange Range;
	Range.setAspectMask(view_aspect_flags)
		// TODO: Make configurable
		.setBaseMipLevel(0)
		.setLevelCount(1)
		.setBaseArrayLayer(0)
		.setLayerCount(type == TextureType::eTexture_Type_Cube ? 6 : 1);

	vk::ImageViewCreateInfo ImageViewCreateInfo;
	ImageViewCreateInfo.setImage(Image)
		.setFormat(format)
		.setSubresourceRange(Range);

	switch (type)
	{
	default:
	case eTexture_Type_2D:
		ImageViewCreateInfo.setViewType(vk::ImageViewType::e2D);
		break;
	case eTexture_Type_Cube:
		ImageViewCreateInfo.setViewType(vk::ImageViewType::eCube);
		break;
	}

	ImageView = context->Device.GetLogicalDevice().createImageView(ImageViewCreateInfo, context->Allocator);
	ASSERT(ImageView);
}

void VulkanImage::Destroy(VulkanContext* context) {
	vk::Device LogicalDevice = context->Device.GetLogicalDevice();

	if (ImageView) {
		LogicalDevice.destroyImageView(ImageView, context->Allocator);
		ImageView = nullptr;
	}

	if (DeviceMemory) {
		LogicalDevice.freeMemory(DeviceMemory, context->Allocator);
		DeviceMemory = nullptr;
	}

	if (Image) {
		LogicalDevice.destroyImage(Image, context->Allocator);
		Image = nullptr;
	}

	// Report the memory as no longer in-use.
	bool IsDeviceMemory = (MemoryFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) == vk::MemoryPropertyFlagBits::eDeviceLocal;
	Memory::FreeReport(MemoryRequirements.size, IsDeviceMemory ? MemoryType::eMemory_Type_GPU_Local : MemoryType::eMemory_Type_Vulkan);
	Memory::Zero(&MemoryRequirements, sizeof(vk::MemoryRequirements));
}

void VulkanImage::TransitionLayout(VulkanContext* context, TextureType type, VulkanCommandBuffer* command_buffer, vk::ImageLayout old_layout, vk::ImageLayout new_layout) {
	vk::ImageMemoryBarrier Barrier;
	Barrier.setOldLayout(old_layout)
		.setNewLayout(new_layout)
		.setSrcQueueFamilyIndex(context->Device.GetQueueFamilyInfo()->graphics_index)
		.setDstQueueFamilyIndex(context->Device.GetQueueFamilyInfo()->graphics_index)
		.setImage(Image);

	vk::ImageSubresourceRange Range;
	Range.setAspectMask(vk::ImageAspectFlagBits::eColor)
		.setBaseMipLevel(0)
		.setLevelCount(1)
		.setBaseArrayLayer(0)
		.setLayerCount(type == TextureType::eTexture_Type_Cube ? 6 : 1);
	Barrier.setSubresourceRange(Range);

	vk::PipelineStageFlags SrcStage;
	vk::PipelineStageFlags DstStage;

	// Dont't care about the old layout - transition to optimal layout
	if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal) {
		Barrier.setSrcAccessMask(vk::AccessFlagBits::eNone)
			.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);

		// Dont't care what stage the pipeline is in at the start.
		SrcStage = vk::PipelineStageFlagBits::eTopOfPipe;

		// Used for copying
		DstStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (old_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
		// Transition from a transfer destination layout to a shader-readonly layout
		Barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
			.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

		// From a copying stage to.
		SrcStage = vk::PipelineStageFlagBits::eTransfer;

		// The fragment stage.
		DstStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else {
		LOG_FATAL("Unsupported layout transition!");
		return;
	}

	command_buffer->CommandBuffer.pipelineBarrier(SrcStage, DstStage, vk::DependencyFlagBits::eByRegion, 0, nullptr, 0, nullptr, 1, &Barrier);
}

void VulkanImage::CopyFromBuffer(VulkanContext* context, TextureType type, vk::Buffer buffer, VulkanCommandBuffer* command_buffer) {
	// Region to copy
	vk::BufferImageCopy Region;
	Memory::Zero(&Region, sizeof(vk::BufferImageCopy));
	Region.setBufferOffset(0)
		.setBufferImageHeight(0)
		.setBufferRowLength(0);

	// Subresouce
	vk::ImageSubresourceLayers Subresource;
	Subresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
		.setMipLevel(0)
		.setBaseArrayLayer(0)
		.setLayerCount(type == TextureType::eTexture_Type_Cube ? 6 : 1);
	Region.setImageSubresource(Subresource);

	// Extent
	vk::Extent3D Extent;
	Extent.setWidth(Width)
		.setHeight(Height)
		.setDepth(1);
	Region.setImageExtent(Extent);

	command_buffer->CommandBuffer.copyBufferToImage(buffer, Image, vk::ImageLayout::eTransferDstOptimal, 1, &Region);
}