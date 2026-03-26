#include "VulkanDevice.hpp"
#include "VulkanContext.hpp"
#include "Containers/TArray.hpp"

bool VulkanDevice::Create(VulkanContext* context, vk::SurfaceKHR surface) {
	if (context == nullptr) {
		GLOG(Log::eError, "VulkanDevice::Create() Required a valid VulkanContext point.");
		return false;
	}
	Context = context;

	if (!SelectPhysicalDevice(surface)) {
		return false;
	}

	// ── 查询并缓存设备能力（在创建 LogicalDevice 之前）──────────────────
	if (!QueryCapabilities(PhysicalDevice)) {
		GLOG(Log::eWarn, "VulkanDevice::Create() QueryCapabilities failed, continuing with defaults.");
	}

	GLOG(Log::eInfo, "Creating logical device...");

	bool PresentSharesGraphicsQueue = QueueFamilyInfo.graphics_index == QueueFamilyInfo.present_index;
	bool TransferSharesGraphicsQueue = QueueFamilyInfo.graphics_index == QueueFamilyInfo.transfer_index;
	unsigned int IndexCount = 1;
	if (!PresentSharesGraphicsQueue)  IndexCount++;
	if (!TransferSharesGraphicsQueue) IndexCount++;

	TArray<unsigned int> Indices(IndexCount);
	unsigned short Index = 0;
	Indices[Index++] = QueueFamilyInfo.graphics_index;
	if (!PresentSharesGraphicsQueue)  Indices[Index++] = QueueFamilyInfo.present_index;
	if (!TransferSharesGraphicsQueue) Indices[Index++] = QueueFamilyInfo.transfer_index;

	std::vector<vk::DeviceQueueCreateInfo> QueueCreateInfos(IndexCount);
	float QueuePriority = 1.0f;
	for (unsigned int i = 0; i < IndexCount; ++i) {
		QueueCreateInfos[i]
			.setQueueCount(1)
			.setQueueFamilyIndex(Indices[i])
			.setPQueuePriorities(&QueuePriority);
	}

	// ── 基础特性 ──────────────────────────────────────────────────────────
	vk::PhysicalDeviceFeatures DeviceFeatures;
	DeviceFeatures
		.setSamplerAnisotropy(true)
#ifdef DPLATFORM_MACOS
		.setGeometryShader(false)
#else
		.setGeometryShader(true)
#endif
		.setFillModeNonSolid(true);

	// ── 按 Capabilities 按需启用 1.2 特性 ────────────────────────────────
	vk::PhysicalDeviceVulkan12Features Features12;
	if (Capabilities.bBufferDeviceAddress) {
		Features12.setBufferDeviceAddress(true);
		GLOG(Log::eInfo, "Enabling feature: BufferDeviceAddress");
	}
	if (Capabilities.bDescriptorIndexing) {
		Features12.setDescriptorIndexing(true);
		GLOG(Log::eInfo, "Enabling feature: DescriptorIndexing");
	}
	if (Capabilities.bTimelineSemaphore) {
		Features12.setTimelineSemaphore(true);
		GLOG(Log::eInfo, "Enabling feature: TimelineSemaphore");
	}

	// ── 按 Capabilities 按需启用 1.3 特性 ────────────────────────────────
	vk::PhysicalDeviceVulkan13Features Features13;
	if (Capabilities.bDemoteToHelperInvocation) {
		Features13.setShaderDemoteToHelperInvocation(true);
		GLOG(Log::eInfo, "Enabling feature: ShaderDemoteToHelperInvocation");
	}
	if (Capabilities.bDynamicRendering) {
		Features13.setDynamicRendering(true);
		GLOG(Log::eInfo, "Enabling feature: DynamicRendering");
	}

	// ── pNext 链：Features12 → Features13 ────────────────────────────────
	// 只在对应版本支持时挂载，避免传入不支持的结构体
	void* pNextChain = nullptr;

	if (Capabilities.bSupport_1_3) {
		Features12.setPNext(&Features13);
		pNextChain = &Features12;
	}
	else if (Capabilities.bSupport_1_2) {
		pNextChain = &Features12;
	}
	// 1.1 及以下：pNextChain 为 nullptr，只用基础 Features

	vk::PhysicalDeviceFeatures2 DeviceFeatures2;
	DeviceFeatures2
		.setFeatures(DeviceFeatures)
		.setPNext(pNextChain);

	// ── Extensions ───────────────────────────────────────────────────────
	const char* ExtensionName = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

	vk::DeviceCreateInfo DeviceCreateInfo;
	DeviceCreateInfo
		.setQueueCreateInfoCount(IndexCount)
		.setPQueueCreateInfos(QueueCreateInfos.data())
		.setPNext(&DeviceFeatures2)           // 用 Features2 替代 setPEnabledFeatures
		.setEnabledExtensionCount(1)
		.setPEnabledExtensionNames(ExtensionName);

	LogicalDevice = PhysicalDevice.createDevice(DeviceCreateInfo, Context->Allocator);
	ASSERT(LogicalDevice);
	GLOG(Log::eInfo, "Created logical device.");

	GraphicsQueue = LogicalDevice.getQueue(QueueFamilyInfo.graphics_index, 0);
	PresentQueue = LogicalDevice.getQueue(QueueFamilyInfo.present_index, 0);
	TransferQueue = LogicalDevice.getQueue(QueueFamilyInfo.transfer_index, 0);
	GLOG(Log::eInfo, "Queues obtained.");

	// Command pool
	vk::CommandPoolCreateInfo PoolCreateInfo;
	PoolCreateInfo
		.setQueueFamilyIndex(QueueFamilyInfo.graphics_index)
		.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
	GraphicsCommandPool = LogicalDevice.createCommandPool(PoolCreateInfo, Context->Allocator);
	ASSERT(GraphicsCommandPool);
	GLOG(Log::eInfo, "Created graphics command pool.");

	return true;
}

void VulkanDevice::Destroy() {
	GLOG(Log::eInfo, "Releasing physical device resources...");

	if (!SwapchainSupport.formats.empty()) {
		SwapchainSupport.formats.clear();
		SwapchainSupport.format_count = 0;
	}
	if (!SwapchainSupport.present_modes.empty()) {
		SwapchainSupport.present_modes.clear();
		SwapchainSupport.present_mode_count = 0;
	}

	QueueFamilyInfo.graphics_index = INVALID_ID;
	QueueFamilyInfo.compute_index = INVALID_ID;
	QueueFamilyInfo.transfer_index = INVALID_ID;
	QueueFamilyInfo.present_index = INVALID_ID;

	LogicalDevice.destroyCommandPool(GraphicsCommandPool, Context->Allocator);
	LogicalDevice.destroy(Context->Allocator);
}

void VulkanDevice::QuerySwapchainSupport(
	vk::PhysicalDevice device, vk::SurfaceKHR surface, SSwapchainSupportInfo* support_info)
{
	if (device.getSurfaceCapabilitiesKHR(surface, &support_info->capabilities) != vk::Result::eSuccess) {
		GLOG(Log::eError, "Can not get surface capabilities.");
		return;
	}

	if (device.getSurfaceFormatsKHR(surface, &support_info->format_count, nullptr) != vk::Result::eSuccess) {
		GLOG(Log::eError, "Can not get surface formats.");
		return;
	}
	ASSERT(support_info->format_count != 0);
	support_info->formats.resize(support_info->format_count);
	if (device.getSurfaceFormatsKHR(surface, &support_info->format_count,
		support_info->formats.data()) != vk::Result::eSuccess) {
		GLOG(Log::eError, "Can not get surface formats.");
		return;
	}

	if (device.getSurfacePresentModesKHR(surface, &support_info->present_mode_count,
		nullptr) != vk::Result::eSuccess) {
		GLOG(Log::eError, "Can not get present modes.");
		return;
	}
	ASSERT(support_info->present_mode_count != 0);
	support_info->present_modes.resize(support_info->present_mode_count);
	if (device.getSurfacePresentModesKHR(surface, &support_info->present_mode_count,
		support_info->present_modes.data()) != vk::Result::eSuccess) {
		GLOG(Log::eError, "Can not get present modes.");
		return;
	}
}

bool VulkanDevice::SelectPhysicalDevice(vk::SurfaceKHR surface) {
	unsigned int PhysicalDeviceCount = 0;
	TArray<vk::PhysicalDevice> PhysicalDevices;

	if (Context->Instance.enumeratePhysicalDevices(&PhysicalDeviceCount, nullptr) != vk::Result::eSuccess) {
		GLOG(Log::eError, "Enumerate physical devices failed.");
		return false;
	}
	ASSERT(PhysicalDeviceCount != 0);

	PhysicalDevices.Resize(PhysicalDeviceCount);
	if (Context->Instance.enumeratePhysicalDevices(&PhysicalDeviceCount, PhysicalDevices.Data()) != vk::Result::eSuccess) {
		GLOG(Log::eFatal, "No devices support vulkan.");
		return false;
	}

	for (unsigned int i = 0; i < PhysicalDeviceCount; i++) {
		vk::PhysicalDeviceProperties Properties;
		PhysicalDevices[i].getProperties(&Properties);

		vk::PhysicalDeviceFeatures Features;
		PhysicalDevices[i].getFeatures(&Features);

		vk::PhysicalDeviceMemoryProperties Memory;
		PhysicalDevices[i].getMemoryProperties(&Memory);

		bool bSupportDeviceLocalHostVisible = false;
		for (uint32_t j = 0; j < Memory.memoryTypeCount; ++j) {
			if ((Memory.memoryTypes[j].propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible) &&
				(Memory.memoryTypes[j].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)) {
				bSupportDeviceLocalHostVisible = true;
				break;
			}
		}

		DeviceRequirements.graphics = true;
		DeviceRequirements.present = true;
		DeviceRequirements.transfer = true;
		DeviceRequirements.sampler_anisotropy = true;
		DeviceRequirements.discrete_gpu = true;
		DeviceRequirements.device_extensions_name.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		if (MeetsRequirements(PhysicalDevices[i], surface, &Properties, &Features)) {
			GLOG(Log::eInfo, "Selected device: '%s'", Properties.deviceName.data());

			switch (Properties.deviceType) {
			case vk::PhysicalDeviceType::eOther:         GLOG(Log::eInfo, "GPU type: Unknown.");     break;
			case vk::PhysicalDeviceType::eIntegratedGpu: GLOG(Log::eInfo, "GPU type: Integrated.");  break;
			case vk::PhysicalDeviceType::eDiscreteGpu:   GLOG(Log::eInfo, "GPU type: Discrete.");    break;
			case vk::PhysicalDeviceType::eVirtualGpu:    GLOG(Log::eInfo, "GPU type: Virtual GPU."); break;
			case vk::PhysicalDeviceType::eCpu:           GLOG(Log::eInfo, "GPU type: CPU.");         break;
			default: break;
			}

			GLOG(Log::eInfo, "Driver version: %d.%d.%d",
				VK_VERSION_MAJOR(Properties.driverVersion),
				VK_VERSION_MINOR(Properties.driverVersion),
				VK_VERSION_PATCH(Properties.driverVersion));

			for (unsigned int j = 0; j < Memory.memoryHeapCount; ++j) {
				float MemGiB = (float)Memory.memoryHeaps[j].size / 1024.f / 1024.f / 1024.f;
				if (Memory.memoryHeaps[j].flags & vk::MemoryHeapFlagBits::eDeviceLocal)
					GLOG(Log::eInfo, "Local GPU memory: %.2f GiB.", MemGiB);
				else
					GLOG(Log::eInfo, "Shared System memory: %.2f GiB.", MemGiB);
			}

			PhysicalDevice = PhysicalDevices[i];
			IsSupportDeviceLocalHostVisible = bSupportDeviceLocalHostVisible;
			break;
		}
	}

	if (!PhysicalDevice) {
		GLOG(Log::eError, "No suitable physical device found.");
		return false;
	}

	return true;
}

bool VulkanDevice::MeetsRequirements(
	vk::PhysicalDevice device, vk::SurfaceKHR surface,
	const vk::PhysicalDeviceProperties* properties,
	const vk::PhysicalDeviceFeatures* features)
{
	QueueFamilyInfo.compute_index = INVALID_ID;
	QueueFamilyInfo.graphics_index = INVALID_ID;
	QueueFamilyInfo.present_index = INVALID_ID;
	QueueFamilyInfo.transfer_index = INVALID_ID;

#if defined(DPLATFORM_MACOS)
	if (DeviceRequirements.discrete_gpu) {
		if (properties->deviceType != vk::PhysicalDeviceType::eDiscreteGpu) {
			GLOG(Log::eInfo, "Device is not a discrete GPU, skipping.");
			return false;
		}
	}
#endif

	unsigned int QueueFamilyCount = 0;
	TArray<vk::QueueFamilyProperties> QueueFamilies;
	device.getQueueFamilyProperties(&QueueFamilyCount, nullptr);
	QueueFamilies.Resize(QueueFamilyCount);
	device.getQueueFamilyProperties(&QueueFamilyCount, QueueFamilies.Data());

	GLOG(Log::eInfo, "Graphics | Present | Compute | Transfer | Name");
	short MinTransferScore = 256;
	for (unsigned int i = 0; i < QueueFamilyCount; i++) {
		short CurrentTransferScore = 0;

		if (QueueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
			QueueFamilyInfo.graphics_index = i;
			++CurrentTransferScore;
		}
		if (QueueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute) {
			QueueFamilyInfo.compute_index = i;
			++CurrentTransferScore;
		}
		if (QueueFamilies[i].queueFlags & vk::QueueFlagBits::eTransfer) {
			if (CurrentTransferScore <= MinTransferScore) {
				MinTransferScore = CurrentTransferScore;
				QueueFamilyInfo.transfer_index = i;
			}
		}

		vk::Bool32 SupportedPresent = VK_FALSE;
		if (device.getSurfaceSupportKHR(i, surface, &SupportedPresent) != vk::Result::eSuccess) {
			GLOG(Log::eError, "Get surface support failed.");
			return false;
		}
		if (SupportedPresent) {
			QueueFamilyInfo.present_index = i;
		}
	}

	GLOG(Log::eInfo, "    %d    |    %d    |    %d    |    %d     | %s",
		QueueFamilyInfo.graphics_index != INVALID_ID,
		QueueFamilyInfo.present_index != INVALID_ID,
		QueueFamilyInfo.compute_index != INVALID_ID,
		QueueFamilyInfo.transfer_index != INVALID_ID,
		properties->deviceName.data());

	if ((!DeviceRequirements.graphics || QueueFamilyInfo.graphics_index != INVALID_ID) &&
		(!DeviceRequirements.present || QueueFamilyInfo.present_index != INVALID_ID) &&
		(!DeviceRequirements.compute || QueueFamilyInfo.compute_index != INVALID_ID) &&
		(!DeviceRequirements.transfer || QueueFamilyInfo.transfer_index != INVALID_ID))
	{
		GLOG(Log::eDebug, "Device meets queue requirements.");

		QuerySwapchainSupport(device, surface, &SwapchainSupport);

		if (SwapchainSupport.format_count < 1 || SwapchainSupport.present_mode_count < 1) {
			SwapchainSupport.formats.clear();
			SwapchainSupport.present_modes.clear();
			GLOG(Log::eInfo, "Required swapchain support not present, skipping device.");
			return false;
		}

		// Extension 检查
		if (!DeviceRequirements.device_extensions_name.empty()) {
			unsigned int AvailableExtCount = 0;
			TArray<vk::ExtensionProperties> AvailableExts;
			if (device.enumerateDeviceExtensionProperties(nullptr, &AvailableExtCount, nullptr) != vk::Result::eSuccess) {
				GLOG(Log::eInfo, "Failed to enumerate device extension properties.");
				return false;
			}
			AvailableExts.Resize(AvailableExtCount);
			ASSERT(device.enumerateDeviceExtensionProperties(nullptr, &AvailableExtCount, AvailableExts.Data()) == vk::Result::eSuccess);

			for (auto& Required : DeviceRequirements.device_extensions_name) {
				bool Found = false;
				for (unsigned int j = 0; j < AvailableExtCount; j++) {
					if (strcmp(Required, AvailableExts[j].extensionName) == 0) {
						Found = true;
						break;
					}
				}
				if (!Found) {
					GLOG(Log::eInfo, "Required extension not found: '%s', skipping device.", Required);
					continue;
				}
			}
		}

		if (DeviceRequirements.sampler_anisotropy && !features->samplerAnisotropy) {
			GLOG(Log::eInfo, "Device does not support samplerAnisotropy, skipping.");
			return false;
		}

		return true;
	}

	return false;
}

bool VulkanDevice::DetectDepthFormat() {
	const vk::Format Candidates[] = {
		vk::Format::eD32Sfloat,
		vk::Format::eD32SfloatS8Uint,
		vk::Format::eD24UnormS8Uint
	};
	const unsigned char Sizes[] = { 4, 4, 3 };

	vk::FormatFeatureFlags Flags = vk::FormatFeatureFlagBits::eDepthStencilAttachment;
	for (uint32_t i = 0; i < 3; ++i) {
		vk::FormatProperties Props = PhysicalDevice.getFormatProperties(Candidates[i]);
		if ((Props.linearTilingFeatures & Flags) == Flags ||
			(Props.optimalTilingFeatures & Flags) == Flags)
		{
			DepthFormat = Candidates[i];
			DepthChannelCount = Sizes[i];
			return true;
		}
	}
	return false;
}

bool VulkanDevice::QueryCapabilities(vk::PhysicalDevice physDevice) {
	vk::PhysicalDeviceProperties Props = physDevice.getProperties();
	Capabilities.ApiVersion = Props.apiVersion;
	Capabilities.bSupport_1_1 = VK_API_VERSION_MINOR(Props.apiVersion) >= 1;
	Capabilities.bSupport_1_2 = VK_API_VERSION_MINOR(Props.apiVersion) >= 2;
	Capabilities.bSupport_1_3 = VK_API_VERSION_MINOR(Props.apiVersion) >= 3;

	// pNext 链查询：12 → 13
	vk::PhysicalDeviceVulkan13Features Features13;
	vk::PhysicalDeviceVulkan12Features Features12;
	Features12.setPNext(&Features13);

	vk::PhysicalDeviceFeatures2 Features2;
	Features2.setPNext(&Features12);
	physDevice.getFeatures2(&Features2);

	Capabilities.bDemoteToHelperInvocation = Features13.shaderDemoteToHelperInvocation;
	Capabilities.bDynamicRendering = Features13.dynamicRendering;
	Capabilities.bDescriptorIndexing = Features12.descriptorIndexing;
	Capabilities.bTimelineSemaphore = Features12.timelineSemaphore;
	Capabilities.bBufferDeviceAddress = Features12.bufferDeviceAddress;

	GLOG(Log::eInfo, "-------------- Device Capabilities ---------------");
	GLOG(Log::eInfo, "  Vulkan API:                  %d.%d.%d",
		VK_API_VERSION_MAJOR(Props.apiVersion),
		VK_API_VERSION_MINOR(Props.apiVersion),
		VK_API_VERSION_PATCH(Props.apiVersion));
	GLOG(Log::eInfo, "  DynamicRendering:            %s", Capabilities.bDynamicRendering ? "Yes" : "No");
	GLOG(Log::eInfo, "  DemoteToHelperInvocation:    %s", Capabilities.bDemoteToHelperInvocation ? "Yes" : "No");
	GLOG(Log::eInfo, "  DescriptorIndexing:          %s", Capabilities.bDescriptorIndexing ? "Yes" : "No");
	GLOG(Log::eInfo, "  TimelineSemaphore:           %s", Capabilities.bTimelineSemaphore ? "Yes" : "No");
	GLOG(Log::eInfo, "  BufferDeviceAddress:         %s", Capabilities.bBufferDeviceAddress ? "Yes" : "No");
	GLOG(Log::eInfo, "--------------------------------------------------");

	return true;
}