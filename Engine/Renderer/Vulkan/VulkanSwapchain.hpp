#pragma once

#include "vulkan/vulkan.hpp"
#include "Renderer/RendererTypes.hpp"

class VulkanContext;
class VulkanFrameBuffer;
class Texture;

class VulkanSwapchain {
public:
	VulkanSwapchain() : MaxFramesInFlight(0), ImageCount(0){}

public:
	void Create(VulkanContext* context, unsigned int width, unsigned int height);
	void Recreate(VulkanContext* context, unsigned int width, unsigned int height);
	bool Destroy(VulkanContext* context);

	void Presnet(VulkanContext* context, vk::Queue present_queue, vk::Semaphore render_complete_semaphore, uint32_t present_image_index);
	uint32_t AcquireNextImageIndex(VulkanContext* context, size_t timeout_ns, vk::Semaphore image_available_semaphore, vk::Fence fence);

public:
	uint32_t MaxFramesInFlight;
	uint32_t ImageCount;

	vk::SurfaceFormatKHR ImageFormat;
	vk::SwapchainKHR Handle;
	std::vector<Texture> RenderTextures;

	std::vector<Texture> DepthTexture;
	
	// Framebuffers used for on-screen rendering
	RenderTarget RenderTargets[3];

};
