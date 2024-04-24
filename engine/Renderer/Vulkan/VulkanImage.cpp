#include "VulkanImage.hpp"

#include "VulkanContext.hpp"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

void VulkanImage::CreateImage(VulkanContext* context, vk::ImageType type, uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
	vk::ImageUsageFlags usage, vk::MemoryPropertyFlags memory_flags, bool create_view, vk::ImageAspectFlags view_aspect_flags) {
	Width = width;
	Height = height;

	vk::Device LogicalDevice = context->Device.GetLogicalDevice();

	vk::Extent3D Extent;
	Extent.setWidth(Width)
		.setHeight(Height)
		.setDepth(1);			// TODO: Support configurable depth

	vk::ImageCreateInfo ImageCreateInfo;
	ImageCreateInfo.setImageType(vk::ImageType::e2D)
		.setExtent(Extent)
		.setMipLevels(4)		// TODO: Support mip mapping
		.setArrayLayers(1)		// TODO: Support number of layers in the image
		.setFormat(format)
		.setTiling(tiling)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setUsage(usage)
		.setSamples(vk::SampleCountFlagBits::e1)		// TODO: Configurable sample count
		.setSharingMode(vk::SharingMode::eExclusive);	// TODO: Configurable sharing mode

	Image = LogicalDevice.createImage(ImageCreateInfo, context->Allocator);
	ASSERT(Image);

	// Query memory requirements
	vk::MemoryRequirements MemoryRequirements;
	MemoryRequirements = LogicalDevice.getImageMemoryRequirements(Image);

	uint32_t MemoryType = context->FindMemoryIndex(MemoryRequirements.memoryTypeBits, memory_flags);
	if (MemoryType == -1) {
		UL_ERROR("Required memory type not found. Image not vaild.");
	}

	// Allocate memory
	vk::MemoryAllocateInfo MemoryAllocateInfo;
	MemoryAllocateInfo.setAllocationSize(MemoryRequirements.size)
		.setMemoryTypeIndex(MemoryType);
	DeviceMemory = LogicalDevice.allocateMemory(MemoryAllocateInfo, context->Allocator);
	ASSERT(DeviceMemory);

	// Bind memory
	LogicalDevice.bindImageMemory(Image, DeviceMemory, 0);

	// Create image view
	if (create_view) {
		CreateImageView(context, format, view_aspect_flags);
	}

}

void VulkanImage::CreateImageView(VulkanContext* context, vk::Format format, vk::ImageAspectFlags view_aspect_flags) {
	vk::ImageSubresourceRange Range;
	Range.setAspectMask(view_aspect_flags)
		// TODO: Make configurable
		.setBaseMipLevel(0)
		.setLevelCount(1)
		.setBaseArrayLayer(0)
		.setLayerCount(1);

	vk::ImageViewCreateInfo ImageViewCreateInfo;
	ImageViewCreateInfo.setImage(Image)
		.setViewType(vk::ImageViewType::e2D)
		.setFormat(format)
		.setSubresourceRange(Range);

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
}