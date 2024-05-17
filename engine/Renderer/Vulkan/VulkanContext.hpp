#pragma once
#include <vulkan/vulkan.hpp>

#include "Defines.hpp"
#include "VulkanDevice.hpp"
#include "VulkanSwapchain.hpp"
#include "VulkanRenderpass.hpp"
#include "VulkanCommandBuffer.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanShader.hpp"

#include "Systems/GeometrySystem.h"

class Texture;

class VulkanContext {
public:
	VulkanContext(): Allocator(nullptr), Shaders(nullptr), GraphicsCommandBuffers(nullptr){}
	virtual ~VulkanContext() {}

public:
	virtual uint32_t FindMemoryIndex(uint32_t type_filter, vk::MemoryPropertyFlags property_flags) {
		vk::PhysicalDeviceMemoryProperties MemoryProperties;
		MemoryProperties = Device.GetPhysicalDevice().getMemoryProperties();

		for (uint32_t i = 0; i < MemoryProperties.memoryTypeCount; ++i) {
			if (type_filter & (1 << i) && (MemoryProperties.memoryTypes[i].propertyFlags & property_flags) == property_flags) {
				return i;
			}
		}

		UL_WARN("Unable to find suitable memory type!");
		return -1;
	}


public:
	double FrameDeltaTime;

	uint32_t FrameBufferWidth;
	uint32_t FrameBufferHeight;

	// Current generation of framebuffer size. If it does not match framebuffer_size_last_generation then should be generated
	size_t FramebufferSizeGenerate;
	// The generation of the framebuffer when it was last created. Set to framebuffer_size_generation when updated.
	size_t FramebufferSizeGenerateLast;

	uint32_t ImageIndex;
	uint32_t CurrentFrame;
	bool RecreatingSwapchain;

	vk::Instance Instance;
	vk::AllocationCallbacks* Allocator;
	vk::SurfaceKHR Surface;

#ifdef LEVEL_DEBUG
	vk::DebugUtilsMessengerEXT DebugMessenger;
#endif

	VulkanCommandBuffer* GraphicsCommandBuffers;

	std::vector<vk::Semaphore> ImageAvailableSemaphores;
	std::vector<vk::Semaphore> QueueCompleteSemaphores;

	uint32_t InFlightFenceCount;
	std::vector <vk::Fence> InFlightFences;

	// Holds pointers to Fences which exist and are owned elsewhere
	std::vector <vk::Fence*> ImagesInFilght;

	VulkanDevice Device;
	VulkanSwapchain Swapchain;
	VulkanRenderPass MainRenderPass;
	VulkanRenderPass UIRenderPass;

	// Shaders
	Shader MaterialShader;
	Shader UIShader;
	Shader* Shaders;
	uint32_t MaxShaderCount;

	// Geometry
	VulkanBuffer ObjectVertexBuffer;
	VulkanBuffer ObjectIndexBuffer;

	// Framebuffers used for world rendering, one per frame.
	vk::Framebuffer WorldFramebuffers[3];

	// TODO: Make dynamic
	GeometryData Geometries[GEOMETRY_MAX_COUNT];

};
