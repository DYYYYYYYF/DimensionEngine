#pragma once

#include "vulkan/vulkan.hpp"
#include "Core/EngineLogger.hpp"

struct  SVulkanPhysicalDeviceRequirements {
	bool graphics;
	bool present;
	bool compute;
	bool transfer;

	std::vector<const char*> device_extensions_name;
	bool sampler_anisotropy;
	bool discrete_gpu;
};

struct SVulkanPhysicalDeviceQueueFamilyInfo {
	uint32_t graphics_index;
	uint32_t present_index;
	uint32_t compute_index;
	uint32_t transfer_index;
};

struct SSwapchainSupportInfo {
	vk::SurfaceCapabilitiesKHR capabilities;
	unsigned int format_count;
	std::vector<vk::SurfaceFormatKHR> formats;
	unsigned int present_mode_count;
	std::vector<vk::PresentModeKHR> present_modes;
};

class VulkanDevice {
public:
	bool Create(vk::Instance* instance, vk::AllocationCallbacks* allocator, vk::SurfaceKHR surface);
	void Destroy(vk::Instance* instance);

public:
	// Physical & logical device
	vk::PhysicalDevice GetPhysicalDevice() { return PhysicalDevice; }
	vk::Device GetLogicalDevice() { return LogicalDevice; }

	// Device properties
	SVulkanPhysicalDeviceRequirements* GetDeviceRequirements() { return &DeviceRequirements; }
	SVulkanPhysicalDeviceQueueFamilyInfo* GetQueueFamilyInfo() { return &QueueFamilyInfo; }
	const SVulkanPhysicalDeviceRequirements* GetDeviceRequirements() const { return &DeviceRequirements; }
	const SVulkanPhysicalDeviceQueueFamilyInfo* GetQueueFamilyInfo() const { return &QueueFamilyInfo; }

	// Swapchain support
	SSwapchainSupportInfo* GetSwapchainSupportInfo() { return &SwapchainSupport; }
	const SSwapchainSupportInfo* GetSwapchainSupportInfo() const { return &SwapchainSupport; }

private:
	bool SelectPhysicalDevice(vk::Instance* instance, vk::SurfaceKHR surface);
	bool MeetsRequirements(vk::PhysicalDevice device, vk::SurfaceKHR surface, const vk::PhysicalDeviceProperties* properties,
		const vk::PhysicalDeviceFeatures* features);

	void QuerySwapchainSupport(vk::PhysicalDevice device, vk::SurfaceKHR surface, SSwapchainSupportInfo* support_info);

private:
	vk::PhysicalDevice PhysicalDevice;
	vk::Device LogicalDevice;
	vk::Queue GraphicsQueue;
	vk::Queue PresentQueue;
	vk::Queue TransferQueue;

	SVulkanPhysicalDeviceRequirements DeviceRequirements;
	SVulkanPhysicalDeviceQueueFamilyInfo QueueFamilyInfo;
	SSwapchainSupportInfo SwapchainSupport;
};
