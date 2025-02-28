#include "VulkanDevice.hpp"
#include "VulkanContext.hpp"
#include "Containers/TArray.hpp"

bool VulkanDevice::Create(VulkanContext* context, vk::SurfaceKHR surface) {
	if (context == nullptr) {
		LOG_ERROR("VulkanDevice::Create() Required a valid VulkanContext point.");
		return false;
	}
	Context = context;
	
	if (!SelectPhysicalDevice(surface)) {
		return false;
	}

	LOG_INFO("Creating logical device...");
	// NOTE: Do not create additional queues for shared indices.
	bool PresentSharesGraphicsQueue = QueueFamilyInfo.graphics_index == QueueFamilyInfo.present_index;
	bool TransferSharesGraphicsQueue = QueueFamilyInfo.graphics_index == QueueFamilyInfo.transfer_index;
	unsigned int IndexCount = 1;
	if (!PresentSharesGraphicsQueue) {
		IndexCount++;
	}
	if (!TransferSharesGraphicsQueue) {
		IndexCount++;
	}

	TArray<unsigned int> Indices(IndexCount);
	unsigned short Index = 0;
	Indices[Index++] = QueueFamilyInfo.graphics_index;
	if (!PresentSharesGraphicsQueue) {
		Indices[Index++] = QueueFamilyInfo.present_index;
	}

	if (!TransferSharesGraphicsQueue) {
		Indices[Index++] = QueueFamilyInfo.transfer_index;
	}

	std::vector<vk::DeviceQueueCreateInfo> QueueCreateInfos(IndexCount);
	for (unsigned int i = 0; i < IndexCount; ++i) {
		QueueCreateInfos[i].setQueueCount(1)
			.setQueueFamilyIndex(Indices[i]);
		//if (Indices[i] == QueueFamilyInfo.graphics_index) {
		//	QueueCreateInfos[i].setQueueCount(2);
		//}

		float QueuePriority = 1.0f;
		QueueCreateInfos[i].setPQueuePriorities(&QueuePriority);
	}

	// Request device features
	// TODO: should be config driven
	vk::PhysicalDeviceFeatures DeviceFeatures;
	DeviceFeatures.setSamplerAnisotropy(true)	// It will not append on modern device, but not sure.
		.setGeometryShader(true)
		.setFillModeNonSolid(true);	// It should be true when Pipeline's PolygonMode set on eLine or ePoint.

	const char* ExtensionName = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
	vk::DeviceCreateInfo DeviceCreateInfo;
	DeviceCreateInfo.setQueueCreateInfoCount(IndexCount)
		.setPQueueCreateInfos(QueueCreateInfos.data())
		.setPEnabledFeatures(&DeviceFeatures)
		.setEnabledExtensionCount(1)
		.setPEnabledExtensionNames(ExtensionName);

	LogicalDevice = PhysicalDevice.createDevice(DeviceCreateInfo, Context->Allocator);
	ASSERT(LogicalDevice);
	LOG_INFO("Created logical device.");

	GraphicsQueue = LogicalDevice.getQueue(QueueFamilyInfo.graphics_index, 0);
	PresentQueue = LogicalDevice.getQueue(QueueFamilyInfo.present_index, 0);
	TransferQueue = LogicalDevice.getQueue(QueueFamilyInfo.transfer_index, 0);
	LOG_INFO("Queues obtained.");

	// Create command pool for graphics queue
	vk::CommandPoolCreateInfo PoolCreateInfo;
	PoolCreateInfo.setQueueFamilyIndex(QueueFamilyInfo.graphics_index)
		.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
	GraphicsCommandPool = LogicalDevice.createCommandPool(PoolCreateInfo, Context->Allocator);
	ASSERT(GraphicsCommandPool);
	LOG_INFO("Created graphics command pool.");

	return true;
}

void VulkanDevice::Destroy() {

	LOG_INFO("Releasing physical device resources...");
	if (!SwapchainSupport.formats.empty()) {
		SwapchainSupport.formats.clear();
		SwapchainSupport.format_count = 0;
	}

	if (!SwapchainSupport.present_modes.empty()) {
		SwapchainSupport.present_modes.clear();
		SwapchainSupport.present_mode_count = 0;
	}

	QueueFamilyInfo.graphics_index = -1;
	QueueFamilyInfo.compute_index = -1;
	QueueFamilyInfo.transfer_index = -1;
	QueueFamilyInfo.present_index = -1;

	LogicalDevice.destroyCommandPool(GraphicsCommandPool, Context->Allocator);
	LogicalDevice.destroy(Context->Allocator);
}

void VulkanDevice::QuerySwapchainSupport(vk::PhysicalDevice device, vk::SurfaceKHR surface, SSwapchainSupportInfo* support_info) {
	// Surface capabilities
	if (device.getSurfaceCapabilitiesKHR(surface, &support_info->capabilities) != vk::Result::eSuccess) {
		LOG_ERROR("Can not get surface capalities.");
		return;
	}

	// Surface formats;
	if (device.getSurfaceFormatsKHR(surface, &support_info->format_count, nullptr) != vk::Result::eSuccess) {
		LOG_ERROR("Can not get surface formats.");
		return;
	}
	ASSERT(support_info->format_count != 0);

	support_info->formats.resize(support_info->format_count);
	if (device.getSurfaceFormatsKHR(surface, &support_info->format_count,
		support_info->formats.data()) != vk::Result::eSuccess) {
		LOG_ERROR("Can not get surface formats.");
		return;
	}

	// Surface present mode
	if (device.getSurfacePresentModesKHR(surface, &support_info->present_mode_count,
		support_info->present_modes.data()) != vk::Result::eSuccess) {
		LOG_ERROR("Can not get present modes.");
		return;
	}
	ASSERT(support_info->present_mode_count != 0);

	support_info->present_modes.resize(support_info->present_mode_count);
	if (device.getSurfacePresentModesKHR(surface, &support_info->present_mode_count,
		support_info->present_modes.data()) != vk::Result::eSuccess) {
		LOG_ERROR("Can not get surface formats.");
		return;
	}
}

bool VulkanDevice::SelectPhysicalDevice(vk::SurfaceKHR surface) {
	unsigned int PhysicalDeviceCount;
	TArray<vk::PhysicalDevice> PhysicalDevices;

	if (Context->Instance.enumeratePhysicalDevices(&PhysicalDeviceCount, nullptr) != vk::Result::eSuccess) {
		LOG_ERROR("Enumerate physical devices failed.");
		return false;
	}

	ASSERT(PhysicalDeviceCount != 0);
	PhysicalDevices.Resize(PhysicalDeviceCount);
	if (Context->Instance.enumeratePhysicalDevices(&PhysicalDeviceCount, PhysicalDevices.Data()) != vk::Result::eSuccess) {
		LOG_FATAL("No devices support vulkan.");
		return false;
	}

	for (unsigned int i = 0; i < PhysicalDeviceCount; i++) {
		vk::PhysicalDeviceProperties Properties;
		PhysicalDevices[i].getProperties(&Properties);

		vk::PhysicalDeviceFeatures features;
		PhysicalDevices[i].getFeatures(&features);

		vk::PhysicalDeviceMemoryProperties memory;
		PhysicalDevices[i].getMemoryProperties(&memory);

		// Check if device supports local/host visible combo
		bool bSupportDeviceLocalHostVisible = false;
		for (uint32_t i = 0; i < memory.memoryTypeCount; ++i) {
			// Check each memory type to see if its bit is set to one.
			if (
				((memory.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible)) &&
				((memory.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)) 
			){
				bSupportDeviceLocalHostVisible = true;
				break;
			}
		}

		// TODO: these requirements should probably be driven by engine
		// configuration
		DeviceRequirements.graphics = true;
		DeviceRequirements.present = true;
		DeviceRequirements.transfer = true;

		// NOTO: Enable if support
		//requirements.compute = true;
		DeviceRequirements.sampler_anisotropy = true;
		DeviceRequirements.discrete_gpu = true;
		DeviceRequirements.device_extensions_name.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		if (MeetsRequirements(PhysicalDevices[i], surface, &Properties, &features)) {
			LOG_INFO("Selected device: %s", Properties.deviceName);
			switch (Properties.deviceType)
			{
			case vk::PhysicalDeviceType::eOther:
				LOG_INFO("GPU type is Unknown.");
				break;
			case vk::PhysicalDeviceType::eIntegratedGpu:
				LOG_INFO("GPU type is Integrated.");
				break;
			case vk::PhysicalDeviceType::eDiscreteGpu:
				LOG_INFO("GPU type is Discrete.");
				break;
			case vk::PhysicalDeviceType::eVirtualGpu:
				LOG_INFO("GPU type is Virtual GPU.");
				break;
			case vk::PhysicalDeviceType::eCpu:
				LOG_INFO("GPU type is CPU");
				break;
			}

			// GPU Driver version
			LOG_INFO("GOU Driver version: %d.%d.%d",
				VK_VERSION_MAJOR(Properties.driverVersion),
				VK_VERSION_MINOR(Properties.driverVersion),
				VK_VERSION_PATCH(Properties.driverVersion));

			// Vulkan API version
			LOG_INFO("Vulkan API version: %d.%d.%d",
				VK_VERSION_MAJOR(Properties.apiVersion),
				VK_VERSION_MINOR(Properties.apiVersion),
				VK_VERSION_PATCH(Properties.apiVersion));

			// Memory information
			for (unsigned int j = 0; j < memory.memoryHeapCount; ++j) {
				float MemorySizeGib = (((float)memory.memoryHeaps[j].size) / 1024.f / 1024.f / 1024.f);
				if (memory.memoryHeaps[j].flags & vk::MemoryHeapFlagBits::eDeviceLocal) {
					LOG_INFO("Local GPU memory: %.2f Gib.", MemorySizeGib);
				}
				else {
					LOG_INFO("Shared System memory: %.2f Gib.", MemorySizeGib);
				}
			}

			PhysicalDevice = PhysicalDevices[i];

			IsSupportDeviceLocalHostVisible = bSupportDeviceLocalHostVisible;

			break;
		}
	}

	if (!PhysicalDevice) {
		LOG_INFO("Physical device not selected.");
		return false;
	}

	return true;
}

bool VulkanDevice::MeetsRequirements(vk::PhysicalDevice device, vk::SurfaceKHR surface, const vk::PhysicalDeviceProperties* properties,
	const vk::PhysicalDeviceFeatures* features) {

	QueueFamilyInfo.compute_index = -1;
	QueueFamilyInfo.graphics_index = -1;
	QueueFamilyInfo.present_index = -1;
	QueueFamilyInfo.transfer_index = -1;

	// If not MacOS, discrete GPU is not allowed.
#if !defined(DPLATFORM_MACOS)
	if (DeviceRequirements.discrete_gpu) {
		LOG_WARN("MacOS device may be a Discrete Gpu. Allow selected.");
		if (properties->deviceType != vk::PhysicalDeviceType::eDiscreteGpu) {
			LOG_ERROR("Device is not a discrete GPU. one is least. Skipping.");
			return false;
		}
	}
#endif

	unsigned int QueueFamilyCount;
	TArray<vk::QueueFamilyProperties> QueueFamilies;
	device.getQueueFamilyProperties(&QueueFamilyCount, nullptr);
	QueueFamilies.Resize(QueueFamilyCount);
	device.getQueueFamilyProperties(&QueueFamilyCount, QueueFamilies.Data());

	// Look at each queue and see what queues it supports
	LOG_INFO("Graphics | Present | Compute | Transfer | Name");
	short MinTransferScore = 256;
	for (unsigned int i = 0; i < QueueFamilyCount; i++) {
		short CurrrentTransferScore = 0;

		// Graphics queue
		if (QueueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
			QueueFamilyInfo.graphics_index = i;
			++CurrrentTransferScore;
		}

		// Compute queue
		if (QueueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute) {
			QueueFamilyInfo.compute_index = i;
			++CurrrentTransferScore;
		}

		// Transfer queue
		if (QueueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
			// Take the index if it is the current lowest. This increases the liklihood that it is a dedicated transfer queue
			if (CurrrentTransferScore <= MinTransferScore) {
				MinTransferScore = CurrrentTransferScore;
				QueueFamilyInfo.transfer_index = i;
			}
		}

		// Present queue
vk::Bool32 SupportedPresent = VK_FALSE;
if (device.getSurfaceSupportKHR(i, surface, &SupportedPresent) != vk::Result::eSuccess) {
	LOG_ERROR("Get surface support failed.");
	return false;
}
if (SupportedPresent) {
	QueueFamilyInfo.present_index = i;
}
	}

	LOG_INFO("    %d    |    %d    |    %d    |    %d     | %s",
		QueueFamilyInfo.graphics_index != -1,
		QueueFamilyInfo.present_index != -1,
		QueueFamilyInfo.compute_index != -1,
		QueueFamilyInfo.transfer_index != -1,
		properties->deviceName);

	if (
		(!DeviceRequirements.graphics || (DeviceRequirements.graphics && QueueFamilyInfo.graphics_index != -1)) &&
		(!DeviceRequirements.present || (DeviceRequirements.present && QueueFamilyInfo.present_index != -1)) &&
		(!DeviceRequirements.compute || (DeviceRequirements.compute && QueueFamilyInfo.compute_index != -1)) &&
		(!DeviceRequirements.transfer || (DeviceRequirements.transfer && QueueFamilyInfo.transfer_index != -1))) {
		LOG_DEBUG("Device meets queue requirements.");
		LOG_DEBUG("Graphics Family Index: %i", QueueFamilyInfo.graphics_index);
		LOG_DEBUG("Present Family Index: %i", QueueFamilyInfo.present_index);
		LOG_DEBUG("Compute Family Index: %i", QueueFamilyInfo.compute_index);
		LOG_DEBUG("Transfer Family Index: %i", QueueFamilyInfo.transfer_index);

		QuerySwapchainSupport(device, surface, &SwapchainSupport);

		if (SwapchainSupport.format_count < 1 || SwapchainSupport.present_mode_count < 1) {
			if (!SwapchainSupport.formats.empty()) {
				SwapchainSupport.formats.clear();
			}

			if (!SwapchainSupport.present_modes.empty()) {
				SwapchainSupport.present_modes.clear();
			}

			LOG_INFO("Required swapcain support not present, skipping device.");
			return false;
		}

		if (!DeviceRequirements.device_extensions_name.empty()) {
			unsigned int AvailableExtensionCount = 0;
			TArray<vk::ExtensionProperties> AvailbaleExtensions;
			if (device.enumerateDeviceExtensionProperties(nullptr, &AvailableExtensionCount, nullptr) != vk::Result::eSuccess) {
				LOG_INFO("Required device extension preperties failed.");
				return false;
			}

			AvailbaleExtensions.Resize(AvailableExtensionCount);
			ASSERT(device.enumerateDeviceExtensionProperties(nullptr, &AvailableExtensionCount, AvailbaleExtensions.Data()) == vk::Result::eSuccess);

			for (unsigned int i = 0; i < DeviceRequirements.device_extensions_name.size(); i++) {
				bool IsFound = false;
				for (unsigned int j = 0; j < AvailableExtensionCount; j++) {
					if (strcmp(DeviceRequirements.device_extensions_name[i], AvailbaleExtensions[j].extensionName)) {
						IsFound = true;
						break;
					}
				}

				if (!IsFound) {
					LOG_INFO("Required extension not found: %s", DeviceRequirements.device_extensions_name[i]);
					AvailbaleExtensions.Clear();
					return false;
				}
			}

			AvailbaleExtensions.Clear();
		}

		// Sampler anisotropy
		if (DeviceRequirements.sampler_anisotropy && !features->samplerAnisotropy) {
			LOG_INFO("Device does not support samplerAnisotropy. Skipping.");
			return false;
		}

		return true;
	}

	return false;
}

bool VulkanDevice::DetectDepthFormat() {
	// Format candidates
	const size_t CandidateCount = 3;
	vk::Format Candidates[3] = {
		vk::Format::eD32Sfloat,
		vk::Format::eD32SfloatS8Uint,
		vk::Format::eD24UnormS8Uint
	};

	unsigned char Sizes[3] = { 4, 4, 3 };

	vk::FormatFeatureFlags Flags = vk::FormatFeatureFlagBits::eDepthStencilAttachment;
	for (uint32_t i = 0; i < CandidateCount; ++i) {
		vk::FormatProperties Properties;
		Properties = PhysicalDevice.getFormatProperties(Candidates[i]);

		if ((Properties.linearTilingFeatures & Flags) == Flags){
			DepthFormat = Candidates[i];
			DepthChannelCount = Sizes[i];
			return true;
		}
		else if ((Properties.optimalTilingFeatures & Flags) == Flags) {
			DepthFormat = Candidates[i];
			DepthChannelCount = Sizes[i];
			return true;
		}
	}

	return false;
}
