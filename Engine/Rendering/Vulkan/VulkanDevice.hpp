#pragma once

#include "vulkan/vulkan.hpp"
#include "Core/EngineLogger.hpp"

class VulkanContext;

struct  SVulkanPhysicalDeviceRequirements {
	bool graphics = false;
	bool present = false;
	bool compute = false;
	bool transfer = false;
	bool sampler_anisotropy = false;
	bool discrete_gpu = false;

	std::vector<const char*> device_extensions_name;
};

struct SVulkanPhysicalDeviceQueueFamilyInfo {
	uint32_t graphics_index = INVALID_ID;
	uint32_t present_index = INVALID_ID;
	uint32_t compute_index = INVALID_ID;
	uint32_t transfer_index = INVALID_ID;
};

struct SSwapchainSupportInfo {
	vk::SurfaceCapabilitiesKHR capabilities;
	unsigned int format_count = 0;
	std::vector<vk::SurfaceFormatKHR> formats;
	unsigned int present_mode_count = 0;
	std::vector<vk::PresentModeKHR> present_modes;
};

class VulkanDevice {
public:
	bool Create(VulkanContext* context, vk::SurfaceKHR surface);
	void Destroy();

public:
	// Physical & logical device
	vk::PhysicalDevice GetPhysicalDevice() { return PhysicalDevice; }
	vk::Device GetLogicalDevice() { return LogicalDevice; }

	// Device properties
	SVulkanPhysicalDeviceRequirements* GetDeviceRequirements() { return &DeviceRequirements; }
	SVulkanPhysicalDeviceQueueFamilyInfo* GetQueueFamilyInfo() { return &QueueFamilyInfo; }
	const SVulkanPhysicalDeviceRequirements* GetDeviceRequirements() const { return &DeviceRequirements; }
	const SVulkanPhysicalDeviceQueueFamilyInfo* GetQueueFamilyInfo() const { return &QueueFamilyInfo; }

	vk::PhysicalDeviceProperties GetDeviceProperties() const {
		return PhysicalDevice.getProperties();
	}

	// Swapchain support
	SSwapchainSupportInfo* GetSwapchainSupportInfo() { return &SwapchainSupport; }
	const SSwapchainSupportInfo* GetSwapchainSupportInfo() const { return &SwapchainSupport; }

	void QuerySwapchainSupport(vk::PhysicalDevice device, vk::SurfaceKHR surface, SSwapchainSupportInfo* support_info);

	bool DetectDepthFormat();

	void SetDepthFormat(vk::Format format) { DepthFormat = format; }
	vk::Format GetDepthFormat() { return DepthFormat; }

	vk::CommandPool GetGraphicsCommandPool() { return GraphicsCommandPool; }

	vk::Queue GetGraphicsQueue() { return GraphicsQueue; }
	vk::Queue GetPresentQueue() { return PresentQueue; }
	vk::Queue GetTransferQueue() { return TransferQueue; }

	bool GetIsSupportDeviceLocalHostVisible() const { return IsSupportDeviceLocalHostVisible; }
	unsigned char GetDepthChannelCount() const { return DepthChannelCount; }

private:
	bool SelectPhysicalDevice(vk::SurfaceKHR surface);
	bool MeetsRequirements(vk::PhysicalDevice device, vk::SurfaceKHR surface, const vk::PhysicalDeviceProperties* properties,
		const vk::PhysicalDeviceFeatures* features);


private:
	bool IsSupportDeviceLocalHostVisible = false;
	VulkanContext* Context = nullptr;

	vk::PhysicalDevice PhysicalDevice;
	vk::Device LogicalDevice;
	vk::Queue GraphicsQueue;
	vk::Queue PresentQueue;
	vk::Queue TransferQueue;

	vk::CommandPool GraphicsCommandPool;

	SVulkanPhysicalDeviceRequirements DeviceRequirements;
	SVulkanPhysicalDeviceQueueFamilyInfo QueueFamilyInfo;
	SSwapchainSupportInfo SwapchainSupport;

	vk::Format DepthFormat;
	unsigned char DepthChannelCount = 0;
};
