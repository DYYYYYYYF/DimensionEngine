#include "VulkanBackend.hpp"
#include "VulkanPlatform.hpp"
#include "VulkanDevice.hpp"
#include "VulkanImage.hpp"
#include "VulkanAllocator.hpp"

#include "Core/EngineLogger.hpp"
#include "Containers/TArray.hpp"
#include "Platform/Platform.hpp"
#include "Math/MathTypes.hpp"

#include "Resources/Texture.hpp"
#include "Resources/Geometry.hpp"

#include "Systems/MaterialSystem.h"
#include "Systems/GeometrySystem.h"
#include "Systems/ResourceSystem.h"
#include "Systems/TextureSystem.h"
#include "Systems/ShaderSystem.h"

/*
* Vulkan debug callback
*/
VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT message_servity,
	VkDebugUtilsMessageTypeFlagsEXT message_type,
	const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
	void* user_data
) {
	switch (message_servity) {
	default:
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		LOG_ERROR(callback_data->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		LOG_WARN(callback_data->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		LOG_FATAL(callback_data->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		LOG_INFO(callback_data->pMessage);
		break;
	}

	return VK_FALSE;
}


VulkanBackend::VulkanBackend() {

	Context.Allocator = nullptr;

	// TODO: Implement muti-thread
	Context.EnableMultithreading = false;
	Context.FrameBufferWidth = 800;
	Context.FrameBufferHeight = 600;
}

VulkanBackend::~VulkanBackend() {

}

bool VulkanBackend::Initialize(const RenderBackendConfig* config, unsigned char* out_window_render_target_count, struct SPlatformState* plat_state) {
	Context.OnRenderTargetRefreshRequired = config->OnRenderTargetRefreshRequired;

	// NOTE: Custom allocator;
#if DVULKAN_USE_CUSTOM_ALLOCATOR == 1
	Context.Allocator = (vk::AllocationCallbacks*)Memory::Allocate(sizeof(vk::AllocationCallbacks), MemoryType::eMemory_Type_Renderer);
	if (Context.Allocator != nullptr) {
		if (!VulkanAllocator::Create(Context.Allocator, &Context)) {
			LOG_WARN("Failed to create custom vulkan allocator! Continuing using the default allocator.");
			Memory::Free(Context.Allocator, sizeof(vk::AllocationCallbacks), MemoryType::eMemory_Type_Application);
			Context.Allocator = nullptr;
		}
	}
#endif

	vk::ApplicationInfo ApplicationInfo;
	vk::InstanceCreateInfo InstanceInfo;

	ApplicationInfo.setApiVersion(VK_API_VERSION_1_3)
		.setApplicationVersion(1)
		.setPApplicationName(config->application_name)
		.setEngineVersion(1)
		.setPEngineName("Dimension Engine");
	InstanceInfo.setPApplicationInfo(&ApplicationInfo);

	// Obtain a list of required extensions
	std::vector<const char*> RequiredExtensions;
	RequiredExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	GetPlatformRequiredExtensionNames(RequiredExtensions);
#ifdef LEVEL_DEBUG
	RequiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	std::string output = "\nRequeired extensions : \n";
	for (int i = 0; i < RequiredExtensions.size(); i++) {
		const char* temp = RequiredExtensions[i];
		output.append("\t");
		output.append(temp);
		output.append("\n");
	}
	LOG_DEBUG("%s", output.c_str());
#endif

	// Extemsop
	uint32_t AvailableExtensionsCount = 0;
	vk::Result Result = vk::enumerateInstanceExtensionProperties(nullptr, &AvailableExtensionsCount, nullptr);
	if (Result != vk::Result::eSuccess) {
		LOG_FATAL("Enum instance extension properties failed.");
		return false;
	}
	std::vector<vk::ExtensionProperties> AvailableExtensions;
	AvailableExtensions.resize(AvailableExtensionsCount);
	Result = vk::enumerateInstanceExtensionProperties(nullptr, &AvailableExtensionsCount, AvailableExtensions.data());
	ASSERT(Result == vk::Result::eSuccess);

	// Verify all extensions are available.
	for (uint32_t i = 0; i < RequiredExtensions.size(); ++i) {
		bool Found = false;
		for (uint32_t j = 0; j < AvailableExtensionsCount; j++) {
			if (strcmp(RequiredExtensions[i], AvailableExtensions[j].extensionName) == 0) {
				Found = true;
				LOG_INFO("Required extension found: %s.", RequiredExtensions[i]);
				break;
			}
		}

		if (!Found) {
			LOG_FATAL("Required extension is missing: %s!", RequiredExtensions[i]);
			return false;
		}
	}

	AvailableExtensions.clear();
	std::vector<vk::ExtensionProperties>().swap(AvailableExtensions);

	// Validation layers
	std::vector<const char*> RequiredValidationLayerName;

#ifdef LEVEL_DEBUG
	LOG_INFO("Validation layers enabled. Enumerating ...");
	// List of validation layers required
	RequiredValidationLayerName.push_back("VK_LAYER_KHRONOS_validation");

	// Obtain a list of available validation layers
	uint32_t AvailableLayersCount = 0;
	std::vector<vk::LayerProperties> AvailableLayers;
	vk::Result result;
	result = vk::enumerateInstanceLayerProperties(&AvailableLayersCount, AvailableLayers.data());
	if (result != vk::Result::eSuccess) {
		LOG_FATAL("Enum instance layer properties failed.");
		return false;
	}

	AvailableLayers.resize(AvailableLayersCount);
	result = vk::enumerateInstanceLayerProperties(&AvailableLayersCount, AvailableLayers.data());
	ASSERT(result == vk::Result::eSuccess);

	// Verify all required layers are available.
	for (uint32_t i = 0; i < RequiredValidationLayerName.size(); i++) {
		LOG_INFO("Searching for layer: %s...", RequiredValidationLayerName[i]);
		
		bool IsFound = false;
		for (uint32_t j = 0; j < AvailableLayersCount; j++) {
			if (strcmp(RequiredValidationLayerName[i], AvailableLayers[j].layerName.data()) == 0) {
				IsFound = true;
				LOG_INFO("Found.");
				break;
			}
		}

		if (!IsFound) {
			LOG_WARN("Required validation layer is missing: '%s'!", RequiredValidationLayerName[i]);

			// TODO: Remove spec-element.
			RequiredValidationLayerName.clear();
		}
	}

	AvailableLayers.clear();
	std::vector<vk::LayerProperties>().swap(AvailableLayers);
#endif

	// Extensions
	InstanceInfo.setEnabledExtensionCount((uint32_t)RequiredExtensions.size())
		.setPEnabledExtensionNames(RequiredExtensions);
	// Layers
	InstanceInfo.setEnabledLayerCount((uint32_t)RequiredValidationLayerName.size())
		.setPEnabledLayerNames(RequiredValidationLayerName);

#if defined (DPLATFORM_MACOS)
  InstanceInfo.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;    
#endif

	Context.Instance = vk::createInstance(InstanceInfo, Context.Allocator);
	ASSERT(Context.Instance);

#ifdef LEVEL_DEBUG
	LOG_INFO("Create vulkan debugger...");
	
	vk::DebugUtilsMessageSeverityFlagsEXT LogServerity = 
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;
	// | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
	// | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;

	vk::DebugUtilsMessageTypeFlagsEXT MessageType =
#if !defined(DPLATFORM_MACOS)
		vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding |
#endif
		vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
		vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
		vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;

	vk::DebugUtilsMessengerCreateInfoEXT DebugCreateInfo;
	DebugCreateInfo.setMessageSeverity(LogServerity)
		.setMessageType(MessageType)
		.setPUserData(nullptr)
		.setPfnUserCallback(VulkanDebugCallback);

	auto dispatcher = vk::DispatchLoaderDynamic(Context.Instance, vkGetInstanceProcAddr);
	if (Context.Instance.createDebugUtilsMessengerEXT(&DebugCreateInfo,
		Context.Allocator, &Context.DebugMessenger, dispatcher) != vk::Result::eSuccess) {
		LOG_FATAL("Create debug utils messenger failed.");
		return false;
	}

	LOG_INFO("Vulkan debugger created.");
#endif

	// Surface
	LOG_INFO("Creating vulkan surface...");
	if (!PlatformCreateVulkanSurface(plat_state, &Context)) {
		LOG_ERROR("Create platform surface failed.");
		return false;
	}
	LOG_INFO("Vulkan surface created.");

	// Device
	LOG_INFO("Creating vulkan device...");
	if (!Context.Device.Create(&Context, Context.Surface)) {
		LOG_ERROR("Create vulkan device failed.");
		return false;
	}
	LOG_INFO("Vulkan device created.");

	// Swapchain
	Context.Swapchain.Create(&Context, Context.FrameBufferWidth, Context.FrameBufferHeight);

	// Save off the number of images we have as the number of render targets needed.
	*out_window_render_target_count = Context.Swapchain.ImageCount;

	// Hold registered renderpasses.
	for (uint32_t i = 0; i < VULKAN_MAX_REGISTERED_RENDERPASSES; ++i) {
		Context.RegisteredPasses[i].SetID(INVALID_ID_U16);
	}

	// The renderpass table will be a lookup of array indices. Start off every index with an invalid id.
	Context.RenderpassTableBlock = Memory::Allocate(sizeof(uint32_t) * VULKAN_MAX_REGISTERED_RENDERPASSES, MemoryType::eMemory_Type_Renderer);
	Context.RenderpassTable.Create(sizeof(uint32_t), VULKAN_MAX_REGISTERED_RENDERPASSES, Context.RenderpassTableBlock, false);
	uint32_t Value = INVALID_ID;
	Context.RenderpassTable.Fill(&Value);

	// Renderpasses
	for (uint32_t i = 0; i < config->renderpass_count; ++i) {
		// TODO: move to a function for reusability.
		// Make sure there are no collisions with the name first.
		uint32_t ID = INVALID_ID;
		Context.RenderpassTable.Get(config->pass_config[i].name, &ID);
		if (ID != INVALID_ID) {
			LOG_ERROR("Collision with renderpass named '%s'. Initialization failed.", config->pass_config[i].name);
			return false;
		}

		// Snip up a new id.
		for (uint32_t j = 0; j < VULKAN_MAX_REGISTERED_RENDERPASSES; ++j) {
			if (Context.RegisteredPasses[j].GetID() == INVALID_ID_U16) {
				// Found.
				Context.RegisteredPasses[j].SetID(j);
				ID = j;
				break;
			}
		}

		// Verify we got an id
		if (ID == INVALID_ID) {
			LOG_ERROR("No space was found for a new renderpass. Increase VULKAN_MAX_REGISTERED_RENDERPASSES. Initialization failed.");
			return false;
		}

		// Set up renderpass.
		Context.RegisteredPasses[ID].SetClearColor(config->pass_config[i].clear_color);
		Context.RegisteredPasses[ID].SetClearFlags(config->pass_config[i].clear_flags);
		Context.RegisteredPasses[ID].SetRenderArea(config->pass_config[i].render_area);

		bool HasPrev = config->pass_config[i].prev_name != nullptr;
		bool HasNext = config->pass_config[i].next_name != nullptr;
		Context.RegisteredPasses[ID].Create(&Context, 1.0f, 0, HasPrev, HasNext);

		// Update table with the new id.
		Context.RenderpassTable.Set(config->pass_config[i].name, &ID);
	}

	// Command buffers
	CreateCommandBuffer();

	// Sync objects
	Context.ImageAvailableSemaphores.resize(Context.Swapchain.MaxFramesInFlight);
	Context.QueueCompleteSemaphores.resize(Context.Swapchain.MaxFramesInFlight);
	Context.InFlightFences.resize(Context.Swapchain.MaxFramesInFlight);

	for (uint32_t i = 0; i < Context.Swapchain.MaxFramesInFlight; ++i) {
		vk::SemaphoreCreateInfo SeamphoreCreateInfo;
		Context.ImageAvailableSemaphores[i] = Context.Device.GetLogicalDevice().createSemaphore(SeamphoreCreateInfo, Context.Allocator);
		Context.QueueCompleteSemaphores[i] = Context.Device.GetLogicalDevice().createSemaphore(SeamphoreCreateInfo, Context.Allocator);

		// Create the fence in a signaled state, indicating that the first frame has already been rendered
		// This will prevent the application from waiting indefinitely for the first frame to render since it 
		// cannot be rendered until a frame is rendered before it.
		vk::FenceCreateInfo FenceCreateInfo;
		FenceCreateInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
		Context.InFlightFences[i] = Context.Device.GetLogicalDevice().createFence(FenceCreateInfo, Context.Allocator);
	}

	// In flight fences should not yet exist at this point, so clear the list. These are stored in pointers
	// because the initial state should be 0, and will be 0 when not in use. Actual fences are not owned by this list
	Context.ImagesInFilght.resize(Context.Swapchain.ImageCount);
	for (uint32_t i = 0; i < Context.Swapchain.ImageCount; ++i) {
		Context.ImagesInFilght[i] = nullptr;
	}

	// Geometry vertex buffer
	const size_t VertexBufferSize = sizeof(Vertex) * 1024 * 1024;
	if (!CreateRenderbuffer(RenderbufferType::eRenderbuffer_Type_Vertex, VertexBufferSize, true, &Context.ObjectVertexBuffer)) {
		LOG_ERROR("Error creating vertex buffer.");
		return false;
	}
	BindRenderbuffer(&Context.ObjectVertexBuffer, 0);

	// Geometry index buffer
	const size_t IndexBufferSize = sizeof(uint32_t) * 1024 * 1024;
	if (!CreateRenderbuffer(RenderbufferType::eRenderbuffer_Type_Index, IndexBufferSize, true, &Context.ObjectIndexBuffer)) {
		LOG_ERROR("Error creating index buffer.");
		return false;
	}
	BindRenderbuffer(&Context.ObjectIndexBuffer, 0);

	// Mark all geometry as invalid.
	for (uint32_t i = 0; i < GEOMETRY_MAX_COUNT; ++i) {
		Context.Geometries[i].id = INVALID_ID;
	}

	LOG_INFO("Create vulkan instance succeed.");
	return true;
}

void VulkanBackend::Shutdown() {
	vk::Device LogicalDevice = Context.Device.GetLogicalDevice();
	LogicalDevice.waitIdle();

	LOG_DEBUG("Destroying Buffers");
	DestroyRenderbuffer(&Context.ObjectVertexBuffer);
	DestroyRenderbuffer(&Context.ObjectIndexBuffer);

	LOG_DEBUG("Destroying sync objects.");
	for (uint32_t i = 0; i < Context.Swapchain.MaxFramesInFlight; ++i) {
		if (Context.ImageAvailableSemaphores[i]) {
			LogicalDevice.destroySemaphore(Context.ImageAvailableSemaphores[i], Context.Allocator);
		}
		if (Context.QueueCompleteSemaphores[i]) {
			LogicalDevice.destroySemaphore(Context.QueueCompleteSemaphores[i], Context.Allocator);
		}

		if (Context.InFlightFences[i]) {
			LogicalDevice.destroyFence(Context.InFlightFences[i], Context.Allocator);
		}
	}

	Context.ImageAvailableSemaphores.clear();
	Context.QueueCompleteSemaphores.clear();
	Context.InFlightFences.clear();

	LOG_DEBUG("Destroying command buffers.");
	for (uint32_t i = 0; i < Context.Swapchain.ImageCount; ++i) {
		if (Context.GraphicsCommandBuffers[i].CommandBuffer) {
			Context.GraphicsCommandBuffers[i].Free(&Context, Context.Device.GetGraphicsCommandPool());
		}
	}

	LOG_DEBUG("Destroying renderpasses.");
	for (uint32_t i = 0; i < VULKAN_MAX_REGISTERED_RENDERPASSES; ++i) {
		if (Context.RegisteredPasses[i].GetID() != INVALID_ID_U16) {
			DestroyRenderpass(&Context.RegisteredPasses[i]);
		}
	}

	LOG_DEBUG("Destroying swapchain.");
	Context.Swapchain.Destroy(&Context);

	LOG_DEBUG("Destroying vulkan device.");
	Context.Device.Destroy();

	LOG_DEBUG("Destroying vulkan surface.");
	if (Context.Surface) {
		// NOTE: For now, the surface allocated by default vulkan allocator.
		Context.Instance.destroy(Context.Surface, nullptr);
	}

#ifdef LEVEL_DEBUG
	LOG_DEBUG("Destroying vulkan debugger...");
	auto dispatcher = vk::DispatchLoaderDynamic(Context.Instance, vkGetInstanceProcAddr);
	if (Context.DebugMessenger) {
		Context.Instance.destroyDebugUtilsMessengerEXT(Context.DebugMessenger, Context.Allocator, dispatcher);
	}
#endif

	LOG_DEBUG("Destroying vulkan instance...");
	Context.Instance.destroy(Context.Allocator);

	// Destroy the allocator callbacks if set.
	if (Context.Allocator) {
		Memory::Free(Context.Allocator, sizeof(vk::AllocationCallbacks), MemoryType::eMemory_Type_Renderer);
		Context.Allocator = nullptr;
	}
}

bool VulkanBackend::BeginFrame(double delta_time){
	Context.FrameDeltaTime = delta_time;
	VulkanDevice* Device = &Context.Device;

	// Check if recreating swap chain and boot out
	if (Context.RecreatingSwapchain) {
		Device->GetLogicalDevice().waitIdle();
		LOG_INFO("Recreating swapchain, booting.");
		return false;
	}

	// Check if the framebuffer has been resized. If so, a new swapchain must be created.
	if (Context.FramebufferSizeGenerate != Context.FramebufferSizeGenerateLast) {
		Device->GetLogicalDevice().waitIdle();

		// If the swap chain recreation failed
		// boot out before unsetting the flag
		if (!RecreateSwapchain()) {
			LOG_INFO("Recreating swapchain, booting.");
			return false;
		}

		LOG_INFO("Resized, booting.");
		return false;
	}

	// Wait for the execution of the current frame to complete. The fence being free will allow this one to move on
	if (Context.Device.GetLogicalDevice().waitForFences(1, &Context.InFlightFences[Context.CurrentFrame], true, UINT64_MAX)
		!= vk::Result::eSuccess) {
		LOG_WARN("In flight fence wait failure!");
		return false;
	}

	// Acquire the next image from the swap chain. Pass along the semaphore that should signaled when this completes.
	// This same semaphore will later be waited on by the queue submission to ensure this image is available.
	Context.ImageIndex = Context.Swapchain.AcquireNextImageIndex(&Context, UINT64_MAX, Context.ImageAvailableSemaphores[Context.CurrentFrame], nullptr);
	if (Context.ImageIndex == -1) {
		return false;
	}

	// Begin recording commands
	VulkanCommandBuffer* CommandBuffer = &Context.GraphicsCommandBuffers[Context.ImageIndex];
	CommandBuffer->Reset();
	CommandBuffer->BeginCommand(false, false, false);

	// Dynamic state
	vk::Viewport Viewport;
	Viewport.setX(0.0f)
		.setY(0.0f)
		.setWidth((float)Context.FrameBufferWidth)
		.setHeight((float)Context.FrameBufferHeight)
		.setMinDepth(0.0f)
		.setMaxDepth(1.0f);

	// Scissor
	vk::Rect2D Scissor;
	Scissor.setOffset({ 0, 0 })
		.setExtent({ Context.FrameBufferWidth, Context.FrameBufferHeight });

	CommandBuffer->CommandBuffer.setViewport(0, 1, &Viewport);
	CommandBuffer->CommandBuffer.setScissor(0, 1, &Scissor);

	return true;
}

bool VulkanBackend::EndFrame(double delta_time) {

	VulkanCommandBuffer* CommandBuffer = &Context.GraphicsCommandBuffers[Context.ImageIndex];
	
	CommandBuffer->EndCommand();

	// Make sure the previous frame is not using this image
	if (Context.ImagesInFilght[Context.ImageIndex] != VK_NULL_HANDLE) {
		if (Context.Device.GetLogicalDevice().waitForFences(1, Context.ImagesInFilght[Context.ImageIndex], true, UINT64_MAX)
			!= vk::Result::eSuccess) {
			LOG_WARN("In flight fence wait failure!");
			return false;
		}
	}

	// Make sure image fence as in use by this frame
	Context.ImagesInFilght[Context.ImageIndex] = &Context.InFlightFences[Context.CurrentFrame];

	// Reset the fence for use on the next frame
	if (Context.Device.GetLogicalDevice().resetFences(1, &Context.InFlightFences[Context.CurrentFrame])
		!= vk::Result::eSuccess) {
		LOG_WARN("In flight fence wait failure!");
		return false;
	}

	// Submit the queue and wait for the operation to complete
	// Begin queue submission
	vk::SubmitInfo SubmitInfo;
	
	// Command buffer(s) to be executed
	SubmitInfo.setCommandBufferCount(1);
	SubmitInfo.setCommandBuffers(CommandBuffer->CommandBuffer);

	// The semaphore(s) to be signaled then the queue is complete
	SubmitInfo.setSignalSemaphoreCount(1);
	SubmitInfo.setSignalSemaphores(Context.QueueCompleteSemaphores[Context.CurrentFrame]);

	// Wait semaphore ensures that the operation cannot begin until the image is available
	SubmitInfo.setWaitSemaphoreCount(1);
	SubmitInfo.setWaitSemaphores(Context.ImageAvailableSemaphores[Context.CurrentFrame]);

	// Each semaphore waits on the corresponding pipeline stage to complete. 1:1 ratio.
	// vk::PipelineStageFlagBits::eColor_Attachment_Ouput prevents subsequent color attachment.
	// Writes from executing until the semaphore signals
	std::array<vk::PipelineStageFlags,1> Flags = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
	SubmitInfo.setWaitDstStageMask(Flags);

	if (Context.Device.GetGraphicsQueue().submit(1, &SubmitInfo, Context.InFlightFences[Context.CurrentFrame]) != vk::Result::eSuccess) {
		LOG_ERROR("Queue submit failed.");
		return false;
	}

	CommandBuffer->UpdateSubmitted();

	// End queue submission
	Context.Swapchain.Presnet(&Context, Context.Device.GetPresentQueue(),
		Context.QueueCompleteSemaphores[Context.CurrentFrame], Context.ImageIndex);

	return true;
}

void VulkanBackend::Resize(unsigned short width, unsigned short height) {
	// Update the "Framebuffer size generate", a counter which indicates when the
	// framebuffer size has been updated
	Context.FrameBufferWidth = width;
	Context.FrameBufferHeight = height;
	Context.FramebufferSizeGenerate++;

	LOG_INFO("Vulkan renderer backend resize: width/height/generation: %i/%i/%llu", width, height, Context.FramebufferSizeGenerate);
}

void VulkanBackend::CreateCommandBuffer() {
	if (Context.GraphicsCommandBuffers == nullptr) {
		Context.GraphicsCommandBuffers = (VulkanCommandBuffer*)Memory::Allocate(sizeof(VulkanCommandBuffer) * Context.Swapchain.ImageCount, MemoryType::eMemory_Type_Renderer);
		for (uint32_t i = 0; i < Context.Swapchain.ImageCount; ++i) {
			Memory::Zero(&(Context.GraphicsCommandBuffers[i]), sizeof(VulkanCommandBuffer));
		}
	}

	for (uint32_t i = 0; i < Context.Swapchain.ImageCount; ++i) {
		if (Context.GraphicsCommandBuffers[i].CommandBuffer) {
			Context.GraphicsCommandBuffers[i].Free(&Context, Context.Device.GetGraphicsCommandPool());
		}

		Memory::Zero(&Context.GraphicsCommandBuffers[i], sizeof(VulkanCommandBuffer));
		Context.GraphicsCommandBuffers[i].Allocate(&Context, Context.Device.GetGraphicsCommandPool(), true);
	}

	LOG_INFO("Vulkan command buffers created.");
}

bool VulkanBackend::RecreateSwapchain() {
	// If already being recreated, do not try again.
	if (Context.RecreatingSwapchain) {
		LOG_DEBUG("Already called recreate swapchain. Booting.");
		return false;
	}
	
	// Detect if the windows is too small to be drawn to
	if (Context.FrameBufferWidth == 0 || Context.FrameBufferHeight == 0) {
		LOG_DEBUG("Recreate swapchain called when windows is < 1px. Booting.");
		return false;
	}

	// Mark as recreatring if the dimensions are valid.
	Context.RecreatingSwapchain = true;

	// Wait for any operation to complete
	Context.Device.GetLogicalDevice().waitIdle();

	// Clear these out just in case
	for (uint32_t i = 0; i < Context.Swapchain.ImageCount; ++i) {
		Context.ImagesInFilght[i] = nullptr;
	}

	// Requery support
	Context.Device.QuerySwapchainSupport(Context.Device.GetPhysicalDevice(), Context.Surface, Context.Device.GetSwapchainSupportInfo());
	Context.Device.DetectDepthFormat();

	Context.Swapchain.Recreate(&Context, Context.FrameBufferWidth, Context.FrameBufferHeight);

	// Update framebuffer size generation.
	Context.FramebufferSizeGenerateLast = Context.FramebufferSizeGenerate;

	// Cleanup swapcahin
	for (uint32_t i = 0; i < Context.Swapchain.ImageCount; ++i) {
		Context.GraphicsCommandBuffers[i].Free(&Context, Context.Device.GetGraphicsCommandPool());
	}

	// Tell the renderer that a refresh is required.
	if (Context.OnRenderTargetRefreshRequired) {
		Context.OnRenderTargetRefreshRequired();
	}

	CreateCommandBuffer();

	// Clear the recreating flag.
	Context.RecreatingSwapchain = false;

	return true;
}

void VulkanBackend::CreateTexture(const unsigned char* pixels, Texture* texture) {
	// Internal data creation.
	// TODO: Use an allocator for this.
	texture->InternalData = (VulkanImage*)Memory::Allocate(sizeof(VulkanImage), MemoryType::eMemory_Type_Texture);
	VulkanImage* Image = (VulkanImage*)texture->InternalData;
	vk::DeviceSize ImageSize = texture->Width * texture->Height * texture->ChannelCount * (texture->Type == TextureType::eTexture_Type_Cube ? 6 : 1);

	// NOTE: Assumes 8 bits per channel.
	vk::Format ImageFormat = vk::Format::eR8G8B8A8Unorm;

	// NOTE: Lots of assumptions here, different texture types will require.
	// different options here.
	Image->CreateImage(&Context, texture->Type, texture->Width, texture->Height, ImageFormat,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst |
		vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment,
		vk::MemoryPropertyFlagBits::eDeviceLocal,
		true, vk::ImageAspectFlagBits::eColor);

	// Load the data.
	WriteTextureData(texture, 0, (uint32_t)ImageSize, pixels);

	texture->Generation++;
}

void VulkanBackend::DestroyTexture(Texture* texture) {
	Context.Device.GetLogicalDevice().waitIdle();

	VulkanImage* Image = (VulkanImage*)texture->InternalData;

	if (Image != nullptr) {
		Image->Destroy(&Context);
		Memory::Zero(Image, sizeof(VulkanImage));
		Memory::Free(texture->InternalData, sizeof(VulkanImage), MemoryType::eMemory_Type_Texture);
	}

	Memory::Zero(texture, sizeof(Texture));
}

vk::Format VulkanBackend::ChannelCountToFormat(unsigned char channel_count, vk::Format default_format /*= vk::Format::eR8G8B8A8Unorm*/) {
	switch (channel_count)
	{
	case 1:
		return vk::Format::eR8Unorm;
	case 2:
		return vk::Format::eR8G8Unorm;
	case 3:
		return vk::Format::eR8G8B8Unorm;
	case 4:
		return vk::Format::eR8G8B8A8Unorm;
	default:
		return default_format;
	}
}

void VulkanBackend::CreateWriteableTexture(Texture* tex) {
	// Internal data creation.
	tex->InternalData = (VulkanImage*)Memory::Allocate(sizeof(VulkanImage), MemoryType::eMemory_Type_Texture);
	VulkanImage* Image = (VulkanImage*)tex->InternalData;

	vk::Format ImageFormat = ChannelCountToFormat(tex->ChannelCount, vk::Format::eR8G8B8A8Unorm);
	// TODO: Lots of assumptions here, different texture types will require.
	// different options here.
	Image->CreateImage(&Context, tex->Type, tex->Width,  tex->Height, ImageFormat, vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment,
		vk::MemoryPropertyFlagBits::eDeviceLocal, true, vk::ImageAspectFlagBits::eColor);

	tex->Generation++;
}

void VulkanBackend::ResizeTexture(Texture* tex, uint32_t new_width, uint32_t new_height) {
	if (tex == nullptr) {
		return;
	}

	if (tex->InternalData == nullptr) {
		return;
	}

	// Resizing is really just destroying the old image and creating a new one.
	// Data is not preserved because there's no reliable way to map the old data to 
	// the new since the amount of data differs.
	VulkanImage* Image = (VulkanImage*)tex->InternalData;
	Image->Destroy(&Context);

	vk::Format ImageFormat = ChannelCountToFormat(tex->ChannelCount, vk::Format::eR8G8B8A8Unorm);

	// TODO: Lots of assumptions here, different texture types will require different options here.
	Image->CreateImage(&Context, tex->Type, new_width, new_height, ImageFormat, vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment,
		vk::MemoryPropertyFlagBits::eDeviceLocal, true, vk::ImageAspectFlagBits::eColor);

	tex->Generation++;
}

void VulkanBackend::WriteTextureData(Texture* tex, uint32_t offset, uint32_t size, const unsigned char* pixels) {
	VulkanImage* Image = (VulkanImage*)tex->InternalData;

	// Create a staging buffer and load data into it.
	VulkanBuffer Staging;
	if (!CreateRenderbuffer(RenderbufferType::eRenderbuffer_Type_Staging, size, false, &Staging)) {
		LOG_ERROR("Failed to create staging buffer for texture write.");
		return;
	}
	BindRenderbuffer(&Staging, 0);

	LoadRange(&Staging, 0, size, pixels);

	VulkanCommandBuffer TempBuffer;
	vk::CommandPool Pool = Context.Device.GetGraphicsCommandPool();
	vk::Queue Queue = Context.Device.GetGraphicsQueue();
	TempBuffer.AllocateAndBeginSingleUse(&Context, Pool);

	// Transition the layout from whatever it is currently to optimal for reciving data.
	Image->TransitionLayout(&Context, tex->Type, &TempBuffer, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

	// Copy the data from the buffer.
	Image->CopyFromBuffer(&Context, tex->Type, Staging.Buffer, &TempBuffer);

	// Transition from optimal for data reciept to shader-read-only optimal layout.
	Image->TransitionLayout(&Context, tex->Type, &TempBuffer, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

	TempBuffer.EndSingleUse(&Context, Pool, Queue);

	UnBindRenderbuffer(&Staging);
	DestroyRenderbuffer(&Staging);

	tex->Generation++;
}

bool VulkanBackend::CreateGeometry(Geometry* geometry, uint32_t vertex_size, uint32_t vertex_count, 
	const void* vertices, uint32_t index_size, uint32_t index_count, const void* indices) {
	if (vertex_count == 0 || vertices == nullptr) {
		LOG_ERROR("Vulkan renderer create geometry requires vertex data, and none was supplied. vertex_count=%d, vertices=%p", vertex_count, vertices);
		return false;
	}

	// Check if this is a re-upload. If it is, need to free old data afterward.
	bool IsReupload = geometry->InternalID != INVALID_ID;
	GeometryData OldRange;

	GeometryData* InternalData = nullptr;
	if (IsReupload) {
		InternalData = &Context.Geometries[geometry->InternalID];

		// Take a copy of the old range.
		OldRange.index_buffer_offset = InternalData->index_buffer_offset;
		OldRange.index_count = InternalData->index_count;
		OldRange.index_element_size = InternalData->index_element_size;
		OldRange.vertext_buffer_offset = InternalData->vertext_buffer_offset;
		OldRange.vertex_count = InternalData->vertex_count;
		OldRange.vertex_element_size = InternalData->vertex_element_size;
	}
	else {
		for (uint32_t i = 0; i < GEOMETRY_MAX_COUNT; ++i) {
			if (Context.Geometries[i].id == INVALID_ID) {
				// Found a free index.
				geometry->InternalID = i;
				Context.Geometries[i].id = i;
				InternalData = &Context.Geometries[i];
				break;
			}
		}
	}

	if (InternalData == nullptr) {
		LOG_FATAL("Vulkan renderer create geometry failed to find a free index for a new geometry upload. Adjust config to allow for more.");
		return false;
	}

	// Vertex data.
	InternalData->vertex_count = vertex_count;
	InternalData->vertex_element_size = sizeof(Vertex);
	uint32_t VertexTotalSize = vertex_size * vertex_count;
	// Allocate space in the buffer.
	if (!AllocateRenderbuffer(&Context.ObjectVertexBuffer, VertexTotalSize, &InternalData->vertext_buffer_offset)) {
		LOG_ERROR("Vulkan renderer create geometry failed to allocate vertex data.");
		return false;
	}

	if (!LoadRange(&Context.ObjectVertexBuffer, InternalData->vertext_buffer_offset, VertexTotalSize, vertices)) {
		LOG_ERROR("Vulkan renderer create geometry failed to upload vertex data.");
		return false;
	}

	// Index data. If Applicable.
	if (index_count != 0 && indices != nullptr) {
		InternalData->index_count = index_count;
		InternalData->index_element_size = sizeof(uint32_t);
		uint32_t IndexTotalSize = index_size * index_count;
		if (!AllocateRenderbuffer(&Context.ObjectIndexBuffer, IndexTotalSize, &InternalData->index_buffer_offset)) {
			LOG_ERROR("Vulkan renderer create geometry failed to allocate index data.");
			return false;
		}

		if (!LoadRange(&Context.ObjectIndexBuffer, InternalData->index_buffer_offset, IndexTotalSize, indices)) {
			LOG_ERROR("Vulkan renderer create geometry failed to upload index data.");
			return false;
		}
	}

	if (InternalData->generation == INVALID_ID) {
		InternalData->generation = 0;
	}
	else {
		InternalData->generation++;
	}

	if (IsReupload) {
		// Free vertex data.
		FreeRenderbuffer(&Context.ObjectVertexBuffer, OldRange.vertext_buffer_offset, OldRange.vertex_element_size * OldRange.vertex_count);

		// Free index data.
		if (OldRange.index_element_size > 0) {
			FreeRenderbuffer(&Context.ObjectIndexBuffer, OldRange.index_buffer_offset, OldRange.index_element_size * OldRange.index_count);
		}
	}

	return true;
}

void VulkanBackend::DestroyGeometry(Geometry* geometry) {
	if (geometry != nullptr && geometry->InternalID != INVALID_ID) {
		Context.Device.GetLogicalDevice().waitIdle();
		GeometryData* InternalData = &Context.Geometries[geometry->InternalID];

		// Free vertex data.
		FreeRenderbuffer(&Context.ObjectVertexBuffer, InternalData->vertext_buffer_offset, InternalData->vertex_element_size * InternalData->vertex_count);

		// Free index data.
		if (InternalData->index_element_size > 0) {
			FreeRenderbuffer(&Context.ObjectIndexBuffer, InternalData->index_buffer_offset, InternalData->index_element_size * InternalData->index_count);
		}

		// Clean up date.
		Memory::Zero(InternalData, sizeof(GeometryData));
		InternalData->id = INVALID_ID;
		InternalData->generation = INVALID_ID;
	}
}

void VulkanBackend::DrawGeometry(GeometryRenderData* geometry) {
	// Ignore non-uploaded geometries.
	if (geometry->geometry == nullptr) {
		return;
	}

	if (geometry->geometry->InternalID == INVALID_ID) {
		return;
	}

	GeometryData* BufferData = &Context.Geometries[geometry->geometry->InternalID];
	bool IncludIndexData = BufferData->index_count > 0;
	if (!DrawRenderbuffer(&Context.ObjectVertexBuffer, BufferData->vertext_buffer_offset, BufferData->vertex_count, IncludIndexData)) {
		LOG_ERROR("VulkanBackend::DrawGeometry() Failed to draw vertex buffer.");
		return;
	}

	if (IncludIndexData) {
		if (!DrawRenderbuffer(&Context.ObjectIndexBuffer, BufferData->index_buffer_offset, BufferData->index_count, !IncludIndexData)) {
			LOG_ERROR("VulkanBackend::DrawGeometry() Failed to draw index buffer.");
			return;
		}
	}
}

bool VulkanBackend::BeginRenderpass(IRenderpass* pass, RenderTarget* target) {
	pass->Begin(target);
	return true;
}

bool VulkanBackend::EndRenderpass(IRenderpass* pass) {
	pass->End();
	return true;
}

/**
 * Shaders
 */
#ifdef LEVEL_DEBUG
bool VulkanBackend::VerifyShaderID(uint32_t shader_id) {
		if (shader_id == INVALID_ID || Context.Shaders[shader_id].ID == INVALID_ID) {
			return false;
		}

		return true;
}   
#else
bool VulkanBackend::VerifyShaderID(uint32_t shader_id) {
	return true;
}
#endif	// LEVEL_DEBUG

// The index of the global descriptor set.
const uint32_t DESC_SET_INDEX_GLOBAL = 0;
// The index of the instance descriptor set.
const uint32_t DESC_SET_INDEX_INSTANCE = 1;

bool VulkanBackend::CreateShader(Shader* shader, const ShaderConfig* config, IRenderpass* pass, unsigned short stage_count,
	const std::vector<char*>& stage_filenames, std::vector<ShaderStage>& stages) {
	VulkanShader* TS = (VulkanShader*)Memory::Allocate(sizeof(VulkanShader), MemoryType::eMemory_Type_Renderer);
	TS = new(TS)VulkanShader();
	shader->InternalData = TS;

	// Translate stages.
	vk::ShaderStageFlags VkStages[VULKAN_SHADER_MAX_STAGES];
	for (unsigned short i = 0; i < stage_count; ++i) {
		switch (stages[i])
		{
		case eShader_Stage_Fragment:
			VkStages[i] = vk::ShaderStageFlagBits::eFragment;
			break;
		case eShader_Stage_Vertex:
			VkStages[i] = vk::ShaderStageFlagBits::eVertex;
			break;
		case eShader_Stage_Geometry:
			LOG_WARN("vulkan_renderer_shader_create: VK_SHADER_STAGE_GEOMETRY_BIT is set but not yet supported.");
			VkStages[i] = vk::ShaderStageFlagBits::eGeometry;
			break;
		case eShader_Stage_Compute:
			LOG_WARN("vulkan_renderer_shader_create: SHADER_STAGE_COMPUTE is set but not yet supported.");
			VkStages[i] = vk::ShaderStageFlagBits::eCompute;
			break;
		default:
			break;
		}
	}

	// TODO: configurable max descriptor allocate count.

	uint32_t MaxDescriptorAllocateCount = 1024;

	// Take a copy of the pointer to the context.
	VulkanShader* OutShader = (VulkanShader*)shader->InternalData;
	OutShader->Renderpass = (VulkanRenderPass*)pass;
	OutShader->Config.max_descriptor_set_count = MaxDescriptorAllocateCount;

	// Shader stages. Parse out the flags.
	Memory::Zero(OutShader->Config.stages, sizeof(VulkanShaderStageConfig) * VULKAN_SHADER_MAX_STAGES);
	OutShader->Config.stage_count = 0;
	for (uint32_t i = 0; i < stage_count; ++i) {
		// Make sure there is room enough to add the stage.
		if (OutShader->Config.stage_count + 1 > VULKAN_SHADER_MAX_STAGES) {
			LOG_ERROR("Shaders may have a maximum of %d stages", VULKAN_SHADER_MAX_STAGES);
			return false;
		}

		// Make sure the stage is a supported one.
		vk::ShaderStageFlagBits StageFlag;
		switch (stages[i])
		{
		case ShaderStage::eShader_Stage_Vertex:
			StageFlag = vk::ShaderStageFlagBits::eVertex;
			break;
		case ShaderStage::eShader_Stage_Fragment:
			StageFlag = vk::ShaderStageFlagBits::eFragment;
			break;
		default:
			// Go to the next type.
			LOG_ERROR("vulkan_shader_create: Unsupported shader stage flagged: %d. Stage ignored.", stages[i]);
			continue;
		}

		// Set the stage and bump the counter.
		OutShader->Config.stages[OutShader->Config.stage_count].stage = StageFlag;
		strncpy(OutShader->Config.stages[OutShader->Config.stage_count].filename, stage_filenames[i], 255);
		OutShader->Config.stage_count++;
	}

	// Zero out arrays and counts.
	Memory::Zero(OutShader->Config.descriptor_sets, sizeof(VulkanDescriptorSetConfig) * 2);
	OutShader->Config.descriptor_sets[0].sampler_binding_index = INVALID_ID_U8;
	OutShader->Config.descriptor_sets[1].sampler_binding_index = INVALID_ID_U8;

	// Attributes array.
	Memory::Zero(OutShader->Config.attributes, sizeof(vk::VertexInputAttributeDescription) * VULKAN_SHADER_MAX_ATTRIBUTES);

	// Get the uniform count.
	OutShader->GlobalUniformCount = 0;
	OutShader->GlobalUniformSamplerCount = 0;
	OutShader->InstanceUniformCount = 0;
	OutShader->InstanceUniformSamplerCount = 0;
	OutShader->LocalUniformCount = 0;
	uint32_t TotalCount = (uint32_t)config->uniforms.size();
	for (uint32_t i = 0; i < TotalCount; ++i) {
		switch (config->uniforms[i].scope) {
		case ShaderScope::eShader_Scope_Global:
			if (config->uniforms[i].type == ShaderUniformType::eShader_Uniform_Type_Sampler) {
				OutShader->GlobalUniformSamplerCount++;
			}
			else {
				OutShader->GlobalUniformCount++;
			}
			break;
		case ShaderScope::eShader_Scope_Instance:
			if (config->uniforms[i].type == ShaderUniformType::eShader_Uniform_Type_Sampler) {
				OutShader->InstanceUniformSamplerCount++;
			}
			else {
				OutShader->InstanceUniformCount++;
			}
			break;
		case ShaderScope::eShader_Scope_Local:
			OutShader->LocalUniformCount++;
			break;
		}
	}

	// For now, shaders will only ever have these 2 types of descriptor pools.
	OutShader->Config.pool_sizes[0] = vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBuffer, 1024 };
	OutShader->Config.pool_sizes[1] = vk::DescriptorPoolSize{ vk::DescriptorType::eCombinedImageSampler, 4096 };

	// Global descriptor set config.
	if (OutShader->GlobalUniformCount > 0 || OutShader->GlobalUniformSamplerCount > 0) {
		// Global descriptor set config.
		VulkanDescriptorSetConfig* SetConfig = &OutShader->Config.descriptor_sets[OutShader->Config.descriptor_set_count];

		// Global UBO binding is first, if present.
		if (OutShader->GlobalUniformCount > 0) {
			unsigned short BindingIndex = SetConfig->binding_count;
			SetConfig->bindings[BindingIndex].setBinding(BindingIndex);
			SetConfig->bindings[BindingIndex].setDescriptorCount(1);
			SetConfig->bindings[BindingIndex].setDescriptorType(vk::DescriptorType::eUniformBuffer);
			SetConfig->bindings[BindingIndex].setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
			SetConfig->binding_count++;
		}

		// Add a binding for samplers if used.
		if (OutShader->GlobalUniformSamplerCount > 0) {
			unsigned short BindingIndex = SetConfig->binding_count;
			SetConfig->bindings[BindingIndex].setBinding(BindingIndex);
			SetConfig->bindings[BindingIndex].setDescriptorCount(OutShader->GlobalUniformSamplerCount);
			SetConfig->bindings[BindingIndex].setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
			SetConfig->bindings[BindingIndex].setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
			SetConfig->sampler_binding_index = (unsigned char)BindingIndex;
			SetConfig->binding_count++;
		}

		// Increment the set count.
		OutShader->Config.descriptor_set_count++;
	}

	// If using instance uniforms, add a UBO descriptor set.
	if (OutShader->InstanceUniformCount > 0 || OutShader->InstanceUniformSamplerCount > 0) {
		// Global descriptor set config.
		VulkanDescriptorSetConfig* SetConfig = &OutShader->Config.descriptor_sets[OutShader->Config.descriptor_set_count];

		// Global UBO binding is first, if present.
		if (OutShader->InstanceUniformCount > 0) {
			unsigned short BindingIndex = SetConfig->binding_count;
			SetConfig->bindings[BindingIndex].setBinding(BindingIndex);
			SetConfig->bindings[BindingIndex].setDescriptorCount(1);
			SetConfig->bindings[BindingIndex].setDescriptorType(vk::DescriptorType::eUniformBuffer);
			SetConfig->bindings[BindingIndex].setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
			SetConfig->binding_count++;
		}

		// Add a binding for samplers if used.
		if (OutShader->InstanceUniformSamplerCount > 0) {
			unsigned short BindingIndex = SetConfig->binding_count;
			SetConfig->bindings[BindingIndex].setBinding(BindingIndex);
			SetConfig->bindings[BindingIndex].setDescriptorCount(OutShader->InstanceUniformSamplerCount);
			SetConfig->bindings[BindingIndex].setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
			SetConfig->bindings[BindingIndex].setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
			SetConfig->sampler_binding_index = (unsigned char)BindingIndex;
			SetConfig->binding_count++;
		}

		// Increment the set count.
		OutShader->Config.descriptor_set_count++;
	}
	
	// Invalidate all instance states.
	// TODO: Dynamic
	for (uint32_t i = 0; i < 1024; ++i) {
		OutShader->InstanceStates[i].id = INVALID_ID;
		OutShader->InstanceStates[i].offset	= 0;
	}

	// Keep a copy of the cull mode.
	OutShader->Config.cull_mode = config->cull_mode;

	return true;
}

bool VulkanBackend::DestroyShader(Shader* shader) {
	if (shader && shader->InternalData) {
		VulkanShader* Shader = (VulkanShader*)shader->InternalData;
		if (Shader == nullptr) {
			LOG_ERROR("vulkan_renderer_shader_destroy requires a valid pointer to a shader.");
			return false;
		}

		vk::Device LogicalDevice = Context.Device.GetLogicalDevice();
		vk::AllocationCallbacks* VkAllocator = Context.Allocator;

		// Descriptor set layouts.
		for (uint32_t i = 0; i < Shader->Config.descriptor_set_count; ++i) {
			if (Shader->DescriptorSetLayouts[i]) {
				LogicalDevice.destroyDescriptorSetLayout(Shader->DescriptorSetLayouts[i], VkAllocator);
				Shader->DescriptorSetLayouts[i] = nullptr;
			}
		}

		// Descriptor pool
		if (Shader->DescriptorPool) {
			LogicalDevice.destroyDescriptorPool(Shader->DescriptorPool, VkAllocator);
			Shader->DescriptorPool = nullptr;
		}

		// Uniform buffer.
		UnmapMemory(&Shader->UniformBuffer, 0, VK_WHOLE_SIZE);
		Shader->MappedUniformBufferBlock = nullptr;
		DestroyRenderbuffer(&Shader->UniformBuffer);

		// Pipeline
		Shader->Pipeline.Destroy(&Context);

		// Shader modules.
		for (uint32_t i = 0; i < Shader->Config.stage_count; ++i) {
			LogicalDevice.destroyShaderModule(Shader->Stages[i].shader_module, VkAllocator);
		}

		// Destroy the configuration.
		Memory::Zero(&Shader->Config, sizeof(VulkanShaderConfig));

		// Free the internal data memory;
		Memory::Free(shader->InternalData, sizeof(VulkanShader), MemoryType::eMemory_Type_Renderer);
		shader->InternalData = nullptr;

		// Free hash mem.
		shader->UniformLookup.Destroy();
	}

	return true;
}

bool VulkanBackend::InitializeShader(Shader* shader) {
	vk::Device LogicalDevice = Context.Device.GetLogicalDevice();
	vk::AllocationCallbacks* VkAllocator = Context.Allocator;
	VulkanShader* Shader = (VulkanShader*)shader->InternalData;

	// Create a module for each stage.
	Memory::Zero(Shader->Stages, sizeof(VulkanShaderStage) * VULKAN_SHADER_MAX_STAGES);
	for (uint32_t i = 0; i < Shader->Config.stage_count; ++i) {
		if (!CreateModule(Shader, Shader->Config.stages[i], &Shader->Stages[i])) {
			LOG_ERROR("Unable to create %s shader module for '%s'. Shader will be destroyed.", Shader->Config.stages[i].filename, shader->Name);
			return false;
		}
	}

	// Static lookup table for our types->vulkan once.
	static vk::Format* Types = nullptr;
	static vk::Format t[11];
	if (!Types) {
		t[ShaderAttributeType::eShader_Attribute_Type_Float] = vk::Format::eR32Sfloat;
		t[ShaderAttributeType::eShader_Attribute_Type_Float_2] = vk::Format::eR32G32Sfloat;
		t[ShaderAttributeType::eShader_Attribute_Type_Float_3] = vk::Format::eR32G32B32Sfloat;
		t[ShaderAttributeType::eShader_Attribute_Type_Float_4] = vk::Format::eR32G32B32A32Sfloat;
		t[ShaderAttributeType::eShader_Attribute_Type_Int8] = vk::Format::eR8Sint;
		t[ShaderAttributeType::eShader_Attribute_Type_UInt8] = vk::Format::eR8Uint;
		t[ShaderAttributeType::eShader_Attribute_Type_Int16] = vk::Format::eR16Sint;
		t[ShaderAttributeType::eShader_Attribute_Type_UInt16] = vk::Format::eR16Uint;
		t[ShaderAttributeType::eShader_Attribute_Type_Int32] = vk::Format::eR32Sint;
		t[ShaderAttributeType::eShader_Attribute_Type_UInt32] = vk::Format::eR32Uint;
		Types = t;
	}

	// Process attributes.
	uint32_t AttributeCount = (uint32_t)shader->Attributes.size();
	uint32_t Offset = 0;
	for (uint32_t i = 0; i < AttributeCount; ++i) {
		// Setup the new attribute.
		vk::VertexInputAttributeDescription Attribute;
		Attribute.setLocation(i)
			.setBinding(0)
			.setOffset(Offset)
			.setFormat(Types[shader->Attributes[i].type]);

		// Push into the config's attribute collection and add to the stride.
		Shader->Config.attributes[i] = Attribute;
		Offset += shader->Attributes[i].size;
	}

	// Descriptor pool.
	vk::DescriptorPoolCreateInfo PoolInfo;
	PoolInfo.setPoolSizeCount(2)
		.setPPoolSizes(Shader->Config.pool_sizes)
		.setMaxSets(Shader->Config.max_descriptor_set_count)
		.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);

	// Create descriptor pool.
	Shader->DescriptorPool = LogicalDevice.createDescriptorPool(PoolInfo, VkAllocator);
	ASSERT(Shader->DescriptorPool);

	// Create descriptor set layouts.
	Memory::Zero(Shader->DescriptorSetLayouts, sizeof(vk::DescriptorSetLayout) * Shader->Config.descriptor_set_count);
	for (uint32_t i = 0; i < Shader->Config.descriptor_set_count; ++i) {

		vk::DescriptorSetLayoutCreateInfo LayoutInfo;
		LayoutInfo.setBindingCount(Shader->Config.descriptor_sets[i].binding_count)
			.setPBindings(Shader->Config.descriptor_sets[i].bindings);
		Shader->DescriptorSetLayouts[i] = LogicalDevice.createDescriptorSetLayout(LayoutInfo, VkAllocator);
		ASSERT(Shader->DescriptorSetLayouts[i]);
	}

	// TODO: This feels wrong to have these here, at least in this fashion. Should probably
	// Be configured to pull from someplace instead.
	// Viewport.
	vk::Viewport Viewport;
	Viewport.setX(0.0f)
		.setY(0.0f)
		.setWidth((float)Context.FrameBufferWidth)
		.setHeight((float)Context.FrameBufferHeight)
		.setMinDepth(0.0f)
		.setMaxDepth(1.0f);

	// Scissor
	vk::Rect2D Scissor;
	Scissor.setOffset({ 0, 0 })
		.setExtent({ Context.FrameBufferWidth, Context.FrameBufferHeight });

	vk::PipelineShaderStageCreateInfo StageCreateInfos[VULKAN_SHADER_MAX_STAGES];
	Memory::Zero(StageCreateInfos, sizeof(vk::PipelineShaderStageCreateInfo) * VULKAN_SHADER_MAX_STAGES);
	for (uint32_t i = 0; i < Shader->Config.stage_count; ++i) {
		StageCreateInfos[i] = Shader->Stages[i].shader_stage_create_info;
	}

	if (!Shader->Pipeline.Create(&Context, Shader->Renderpass, shader->AttributeStride, (uint32_t)shader->Attributes.size(), Shader->Config.attributes, Shader->Config.descriptor_set_count,
		Shader->DescriptorSetLayouts, Shader->Config.stage_count, StageCreateInfos, Viewport, Scissor, Shader->Config.cull_mode, false, true, shader->PushConstantsRangeCount, shader->PushConstantsRanges)){
		LOG_ERROR("Failed to load graphics pipeline for object shader.");
		return false;
	}

	// Grab the UBO alignment requirement from the device.
	shader->RequiredUboAlignment = Context.Device.GetDeviceProperties().limits.minUniformBufferOffsetAlignment;

	// Make sure the UBO is aligned according to device requirements.
	shader->GlobalUboStride = PaddingAligned(shader->GlobalUboSize, shader->RequiredUboAlignment);
	shader->UboStride = PaddingAligned(shader->UboSize, shader->RequiredUboAlignment);

	// Uniform buffer.
	vk::MemoryPropertyFlags DeviceLocalBits = Context.Device.GetIsSupportDeviceLocalHostVisible() ? vk::MemoryPropertyFlagBits::eDeviceLocal : vk::MemoryPropertyFlags();
	// TODO: max count should be configurable, or perhaps long term support of buffer resizing.
	size_t TotalBufferSize = shader->GlobalUboStride + (shader->UboStride * VULKAN_MAX_MATERIAL_COUNT);
	if (!CreateRenderbuffer(RenderbufferType::eRenderbuffer_Type_Uniform, TotalBufferSize, true, &Shader->UniformBuffer)) {
		LOG_ERROR("Vulkan buffer creation failed for object shader.");
		return false;
	}
	BindRenderbuffer(&Shader->UniformBuffer, 0);

	// Allocate space for the global UBO, which should occupy the _stride_ space, _not_ the actual size used.
	if (!AllocateRenderbuffer(&Shader->UniformBuffer, shader->GlobalUboStride, &shader->GlobalUboOffset)) {
		LOG_ERROR("Failed to allocate space for the uniform buffer!");
		return false;
	}

	// Map the entire buffer's memory.
	Shader->MappedUniformBufferBlock = MapMemory(&Shader->UniformBuffer, 0, TotalBufferSize/*TotalBufferSize*/);

	// Allocate global descriptor sets, one per frame. Global is always the first set.
	vk::DescriptorSetLayout GlobalLayouts[3] = {
		Shader->DescriptorSetLayouts[DESC_SET_INDEX_GLOBAL],
		Shader->DescriptorSetLayouts[DESC_SET_INDEX_GLOBAL],
		Shader->DescriptorSetLayouts[DESC_SET_INDEX_GLOBAL]
	};

	vk::DescriptorSetAllocateInfo AllocInfo;
	AllocInfo.setDescriptorPool(Shader->DescriptorPool)
		.setDescriptorSetCount(3)
		.setPSetLayouts(GlobalLayouts);
	if (LogicalDevice.allocateDescriptorSets(&AllocInfo, Shader->GlobalDescriptorSets)
		!= vk::Result::eSuccess) {
		LOG_ERROR("Allocate descriptor sets failed.");
		return false;
	}

	return true;
}

bool VulkanBackend::UseShader(Shader* shader) {
	if (shader == nullptr) {
		return false;
	}

	VulkanShader* Shader = (VulkanShader*)shader->InternalData;
	Shader->Pipeline.Bind(&Context.GraphicsCommandBuffers[Context.ImageIndex], vk::PipelineBindPoint::eGraphics);
	return true;
}

bool VulkanBackend::BindGlobalsShader(Shader* shader) {
	if (shader == nullptr) {
		return false;
	}

	// Global UBO is always at the beginning, but use this anyway.
	shader->BoundUboOffset = (uint32_t)shader->GlobalUboOffset;
	return true;
}

bool VulkanBackend::BindInstanceShader(Shader* shader, uint32_t instance_id) {
	if (shader == nullptr) {
		LOG_ERROR("vulkan_shader_bind_instance requires a valid pointer to a shader.");
		return false;
	}

	VulkanShader* Internal = (VulkanShader*)shader->InternalData;

	shader->BoundInstanceId = instance_id;
	VulkanShaderInstanceState* ObjectState = &Internal->InstanceStates[instance_id];
	shader->BoundUboOffset = (uint32_t)ObjectState->offset;
	return true;
}

bool VulkanBackend::ApplyGlobalShader(Shader* shader) {
	if (shader == nullptr) {
		return false;
	}

	uint32_t ImageIndex = Context.ImageIndex;
	VulkanShader* Internal = (VulkanShader*)shader->InternalData;
	vk::CommandBuffer CmdBuffer = Context.GraphicsCommandBuffers[ImageIndex].CommandBuffer;
	vk::DescriptorSet GlobalDescriptor = Internal->GlobalDescriptorSets[ImageIndex];

	// Apply UBO first.
	vk::DescriptorBufferInfo BufferInfo;
	BufferInfo.setBuffer(Internal->UniformBuffer.Buffer)
		.setOffset(shader->GlobalUboOffset)
		.setRange(shader->GlobalUboStride);

	// Update descriptor sets.
	vk::WriteDescriptorSet UboWrite;
	UboWrite.setDstSet(Internal->GlobalDescriptorSets[ImageIndex])
		.setDstBinding(0)
		.setDstArrayElement(0)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
		.setDescriptorCount(1)
		.setPBufferInfo(&BufferInfo);

	vk::WriteDescriptorSet DescriptorWrites[2];
	DescriptorWrites[0] = UboWrite;

	uint32_t GlobalSetBindingCount = Internal->Config.descriptor_sets[DESC_SET_INDEX_GLOBAL].binding_count;
	if (GlobalSetBindingCount > 1) {
		// TODO: There are samplers to be written. Support this.
		GlobalSetBindingCount = 1;
		LOG_ERROR("Global image samplers are not yet supported.");

		// VkWriteDescriptorSet sampler_write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
		// descriptor_writes[1] = ...
	}

	Context.Device.GetLogicalDevice().updateDescriptorSets(GlobalSetBindingCount, DescriptorWrites, 0, nullptr);

	// BInd the global descriptor set to be updated.
	CmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, Internal->Pipeline.PipelineLayout, 0, 1, &GlobalDescriptor, 0, nullptr);
	return true;
}

bool VulkanBackend::ApplyInstanceShader(Shader* shader, bool need_update) {
	VulkanShader* Internal = (VulkanShader*)shader->InternalData;
	if (Internal == nullptr) {
		return false;
	}

	if (Internal->InstanceUniformCount < 1 && Internal->InstanceUniformSamplerCount < 1) {
		LOG_ERROR("This shader does not use instances.");
		return false;
	}

	uint32_t ImageIndex = Context.ImageIndex;
	vk::CommandBuffer CmdBuffer = Context.GraphicsCommandBuffers[ImageIndex].CommandBuffer;

	// Obtain instance data.
	VulkanShaderInstanceState* ObjectState = &Internal->InstanceStates[shader->BoundInstanceId];
	vk::DescriptorSet ObjectDescriptorSet = ObjectState->descriptor_set_state.descriptorSets[ImageIndex];

	if (need_update){
		vk::WriteDescriptorSet DescriptorWrites[2];
		Memory::Zero(DescriptorWrites, sizeof(vk::WriteDescriptorSet) * 2);

		uint32_t DescriptorCount = 0;
		uint32_t DescriptorIndex = 0;

		// Descriptor 0 - Uniform buffer
		vk::DescriptorBufferInfo BufferInfo;
		vk::WriteDescriptorSet UboDescriptor;
		if (Internal->InstanceUniformCount > 0) {
			// Only do this if the descriptor has not yet been updated.
			uint32_t* InstanceUboGeneration = &(ObjectState->descriptor_set_state.descriptor_states[DescriptorIndex].generations[ImageIndex]);
			// TODO: determine if update is required.
			if (*InstanceUboGeneration == INVALID_ID/* || *GlobalUboGeneration != Material->Generation*/) {
				BufferInfo.setBuffer(Internal->UniformBuffer.Buffer)
					.setOffset(ObjectState->offset)
					.setRange(shader->UboStride);

				UboDescriptor.setDstSet(ObjectDescriptorSet)
					.setDstBinding(DescriptorIndex)
					.setDescriptorType(vk::DescriptorType::eUniformBuffer)
					.setDescriptorCount(1)
					.setPBufferInfo(&BufferInfo);

				DescriptorWrites[DescriptorCount] = UboDescriptor;
				DescriptorCount++;

				// Update the frame generation. In this case it is only needed once since this is a buffer.
				*InstanceUboGeneration = 1;  // material->generation; TODO: some generation from... somewhere
			}
			DescriptorIndex++;
		}

		// Samplers will always be in the binding. If the binding count is less than 2, there are no samplers.
		vk::DescriptorImageInfo ImageInfos[VULKAN_SHADER_MAX_GLOBAL_TEXTURES];
		vk::WriteDescriptorSet SamplerDescriptor;
		if (Internal->InstanceUniformSamplerCount > 0) {
			// Iterate samplers.
			unsigned char SamplerBindingIndex = Internal->Config.descriptor_sets[DESC_SET_INDEX_INSTANCE].sampler_binding_index;
			uint32_t TotalSamplerCount = Internal->Config.descriptor_sets[DESC_SET_INDEX_INSTANCE].bindings[SamplerBindingIndex].descriptorCount;
			uint32_t UpdateSamplerCount = 0;
			for (uint32_t i = 0; i < TotalSamplerCount; ++i) {
				// TODO: only update in the list if actually needing an update.
				TextureMap* map = Internal->InstanceStates[shader->BoundInstanceId].instance_texture_maps[i];
				if (map == nullptr) {
					continue;
				}

				Texture* t = map->texture;
				if (t == nullptr) {
					continue;
				}

				// Ensure the texture is valid.
				if (t->Generation == INVALID_ID) {
					switch (map->usage)
					{
					case TextureUsage::eTexture_Usage_Map_Diffuse:
						t = TextureSystem::GetDefaultDiffuseTexture();
						break;
					case TextureUsage::eTexture_Usage_Map_Normal:
						t = TextureSystem::GetDefaultNormalTexture();
						break;
					case TextureUsage::eTexture_Usage_Map_Specular:
						t = TextureSystem::GetDefaultSpecularTexture();
						break;
					default:
						LOG_WARN("Undefined texture use %d.", map->usage);
						t = TextureSystem::GetDefaultTexture();
						break;
					}
				}

				VulkanImage* Image = (VulkanImage*)t->InternalData;
				if (Image == nullptr) {
					continue;
				}

				ImageInfos[i].setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
					.setImageView(Image->ImageView)
					.setSampler(*((vk::Sampler*)&map->internal_data));

				// TODO: change up descriptor state to handle this properly.
				// Sync frame generation if not using a default texture.
				/*if (t->generation != INVALID_ID) {
					*descriptor_generation = t->generation;
					*descriptor_id = t->id;
				}*/
				UpdateSamplerCount++;
			}

			SamplerDescriptor.setDstSet(ObjectDescriptorSet)
				.setDstBinding(DescriptorIndex)
				.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
				.setDescriptorCount(UpdateSamplerCount)
				.setPImageInfo(ImageInfos);

			DescriptorWrites[DescriptorCount] = SamplerDescriptor;
			DescriptorCount++;
		}

		if (DescriptorCount > 0) {
			Context.Device.GetLogicalDevice().updateDescriptorSets(DescriptorCount, DescriptorWrites, 0, nullptr);
		}
	}

	// Bind the descriptor set to be updated, or in case the shader changed.
	CmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, Internal->Pipeline.PipelineLayout, 1, 1, &ObjectDescriptorSet, 0, nullptr);
	return true;
}

vk::SamplerAddressMode VulkanBackend::ConvertRepeatType(const char* axis, TextureRepeat repeat) {
	switch (repeat)
	{
	case eTexture_Repeat_Repeat:
		return vk::SamplerAddressMode::eRepeat;
	case eTexture_Repeat_Minrrored_Repeat:
		return vk::SamplerAddressMode::eMirroredRepeat;
	case eTexture_Repeat_Clamp_To_Edge:
		return vk::SamplerAddressMode::eClampToEdge;
	case eTexture_Repeat_Clamp_To_Border:
		return vk::SamplerAddressMode::eClampToBorder;
	default:
		LOG_WARN("Convert repeat type (axis='%s'): Type '%x' not supported, defauting to repeat.", axis, repeat);
		return vk::SamplerAddressMode::eRepeat;
	}
}

vk::Filter VulkanBackend::ConvertFilterType(const char* op, TextureFilter filter) {
	switch (filter)
	{
	case eTexture_Filter_Mode_Nearest:
		return vk::Filter::eNearest;
	case eTexture_Filter_Mode_Linear:
		return vk::Filter::eLinear;
	default:
		LOG_WARN("Convert filter type (op='%s'): Filter '%x' not supported, defauting to linear.", op, filter);
		return vk::Filter::eLinear;
	}
}

bool VulkanBackend::AcquireTextureMap(TextureMap* map) {
	// Create a sampler for the texture.
	vk::SamplerCreateInfo SamplerInfo;
	SamplerInfo.setMinFilter(ConvertFilterType("min", map->filter_minify))
		.setMagFilter(ConvertFilterType("mag", map->filter_magnify))
		.setAddressModeU(ConvertRepeatType("U", map->repeat_u))
		.setAddressModeV(ConvertRepeatType("V", map->repeat_v))
		.setAddressModeW(ConvertRepeatType("W", map->repeat_w));

	// Configurable.
	SamplerInfo.setAnisotropyEnable(VK_TRUE)
		.setMaxAnisotropy(16)
		.setBorderColor(vk::BorderColor::eIntOpaqueBlack)
		.setUnnormalizedCoordinates(VK_FALSE)
		.setCompareEnable(VK_FALSE)
		.setCompareOp(vk::CompareOp::eAlways)
		.setMipmapMode(vk::SamplerMipmapMode::eLinear)
		.setMipLodBias(0.0f)
		.setMinLod(0.0f)
		.setMaxLod(0.0f);

	if (Context.Device.GetLogicalDevice().createSampler(&SamplerInfo, Context.Allocator, (vk::Sampler*)&map->internal_data)
		!= vk::Result::eSuccess) {
		LOG_ERROR("Create sampler failed.");
		return false;
	}

	return true;
}

void VulkanBackend::ReleaseTextureMap(TextureMap* map) {
	if (map) {
		Context.Device.GetLogicalDevice().waitIdle();
		Context.Device.GetLogicalDevice().destroySampler(*((vk::Sampler*)&map->internal_data), Context.Allocator);
		map->internal_data = nullptr;
	}
}

uint32_t VulkanBackend::AcquireInstanceResource(Shader* shader, std::vector<TextureMap*>& maps) {
	VulkanShader* Internal = (VulkanShader*)shader->InternalData;
	// TODO: Dynamic
	uint32_t OutInstanceID = INVALID_ID;
	for (uint32_t i = 0; i < 1024; ++i) {
		if (Internal->InstanceStates[i].id == INVALID_ID) {
			Internal->InstanceStates[i].id = i;
			OutInstanceID = i;
			break;
		}
	}

	if (OutInstanceID == INVALID_ID) {
		LOG_ERROR("vulkan_shader_acquire_instance_resources failed to acquire new id");
		return INVALID_ID;
	}

	VulkanShaderInstanceState* InstanceState = &Internal->InstanceStates[OutInstanceID];
	unsigned char SamplerBindingIndex = Internal->Config.descriptor_sets[DESC_SET_INDEX_INSTANCE].sampler_binding_index;
	uint32_t InstanceTextureCount = Internal->Config.descriptor_sets[DESC_SET_INDEX_INSTANCE].bindings[SamplerBindingIndex].descriptorCount;
	// Wipe out the memory for the entire array, even if it isn't all used.
	InstanceState->instance_texture_maps.resize(shader->InstanceTextureCount);
	Texture* DefaultTexture = TextureSystem::GetDefaultTexture();
	// Set all the texture pointers to default until assigned.
	for (uint32_t i = 0; i < InstanceTextureCount; ++i) {
		InstanceState->instance_texture_maps[i] = maps[i];
		if (!InstanceState->instance_texture_maps[i]->texture) {
			InstanceState->instance_texture_maps[i]->texture = DefaultTexture;
		}
	}

	// Allocate some space in the UBO - by the stride, not the size.
	uint32_t Size = (uint32_t)shader->UboStride;
	if (Size > 0) {
		if (!AllocateRenderbuffer(&Internal->UniformBuffer, Size, &InstanceState->offset)) {
			LOG_ERROR("vulkan_material_shader_acquire_resources failed to acquire ubo space");
			return INVALID_ID;
		}
	}

	VulkanShaderDescriptorSetState* SetState = &InstanceState->descriptor_set_state;

	// Each descriptor binding in the set.
	uint32_t BindingCount = Internal->Config.descriptor_sets[DESC_SET_INDEX_INSTANCE].binding_count;
	Memory::Zero(SetState->descriptor_states, sizeof(VulkanDescriptorState) * VULKAN_SHADER_MAX_BINDINGS);
	for (uint32_t i = 0; i < BindingCount; ++i) {
		for (uint32_t j = 0; j < 3; ++j) {
			SetState->descriptor_states[i].generations[j] = INVALID_ID;
			SetState->descriptor_states[i].ids[j] = INVALID_ID;
		}
	}

	// Allocate 3 descriptor sets (one per frame).
	vk::DescriptorSetLayout Layouts[3] = {
		Internal->DescriptorSetLayouts[DESC_SET_INDEX_INSTANCE],
		Internal->DescriptorSetLayouts[DESC_SET_INDEX_INSTANCE],
		Internal->DescriptorSetLayouts[DESC_SET_INDEX_INSTANCE]
	};

	vk::DescriptorSetAllocateInfo AllocInfo;
	AllocInfo.setDescriptorPool(Internal->DescriptorPool)
		.setDescriptorSetCount(3)
		.setPSetLayouts(Layouts);
	if(Context.Device.GetLogicalDevice().allocateDescriptorSets(&AllocInfo, InstanceState->descriptor_set_state.descriptorSets)
		!= vk::Result::eSuccess) {
			LOG_ERROR("Allocate descriptor sets failed.");
			return INVALID_ID;
	}

	return OutInstanceID;
}

bool VulkanBackend::ReleaseInstanceResource(Shader* shader, uint32_t instance_id) {
	if (shader == nullptr) {
		return false;
	}

	VulkanShader* Internal = (VulkanShader*)shader->InternalData;
	VulkanShaderInstanceState* InstanceState = &Internal->InstanceStates[instance_id];

	// Wait for any pending operations using the descriptor set to finish.
	Context.Device.GetLogicalDevice().waitIdle();

	// Free 3 descriptor sets (one per frame)
	Context.Device.GetLogicalDevice().freeDescriptorSets(Internal->DescriptorPool, 3, InstanceState->descriptor_set_state.descriptorSets);

	// Destroy descriptor states.
	Memory::Zero(InstanceState->descriptor_set_state.descriptor_states, sizeof(VulkanDescriptorState) * VULKAN_SHADER_MAX_BINDINGS);

	if (InstanceState->instance_texture_maps.size() > 0) {
		for (uint32_t i = 0; i < InstanceState->instance_texture_maps.size(); ++i) {
			InstanceState->instance_texture_maps[i]->texture = nullptr;
		}
		InstanceState->instance_texture_maps.clear();
	}

	FreeRenderbuffer(&Internal->UniformBuffer, shader->UboStride, InstanceState->offset);
	InstanceState->offset = INVALID_ID;
	InstanceState->id = INVALID_ID;

	return true;
}

bool VulkanBackend::SetUniform(Shader* shader, ShaderUniform* uniform, const void* value) {
	if (shader == nullptr || uniform == nullptr) {
		return false;
	}

	VulkanShader* Internal = (VulkanShader*)shader->InternalData;
	if (uniform->type == ShaderUniformType::eShader_Uniform_Type_Sampler) {
		if (uniform->scope == ShaderScope::eShader_Scope_Global) {
			shader->GlobalTextureMaps[uniform->location] = (TextureMap*)value;
		}
		else {
			Internal->InstanceStates[shader->BoundInstanceId].instance_texture_maps[uniform->location] = (TextureMap*)value;
		}
	}
	else {
		if (uniform->scope == ShaderScope::eShader_Scope_Local) {
			// Is local, using push constants. Do this immediately.
			vk::CommandBuffer CmdBuffer = Context.GraphicsCommandBuffers[Context.ImageIndex].CommandBuffer;
			CmdBuffer.pushConstants(Internal->Pipeline.PipelineLayout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 
				(uint32_t)uniform->offset, (uint32_t)uniform->size, value);
		}
		else {
			// Map the appropriate memory location and copy the data over.
			size_t Addr = (size_t)Internal->MappedUniformBufferBlock;
			Addr += shader->BoundUboOffset + uniform->offset;
			Memory::Copy((void*)Addr, value, uniform->size);
			if (Addr) {

			}
		}
	}

	return true;
}

bool VulkanBackend::CreateModule(VulkanShader* Shader, VulkanShaderStageConfig config, VulkanShaderStage* shader_stage) {
	// Read the resource.
	Resource BinaryResource;
	if (!ResourceSystem::Load(config.filename, ResourceType::eResource_type_Binary, nullptr, &BinaryResource)) {
		LOG_ERROR("Unable to read shader module: %s.", config.filename);
		return false;
	}

	Memory::Zero(&shader_stage->create_info, sizeof(vk::ShaderModuleCreateInfo));
	shader_stage->create_info.sType = vk::StructureType::eShaderModuleCreateInfo;
	// Use the resource's size and data directly.
	shader_stage->create_info.codeSize = BinaryResource.DataSize;
	shader_stage->create_info.pCode = (uint32_t*)BinaryResource.Data;

	shader_stage->shader_module = Context.Device.GetLogicalDevice().createShaderModule(shader_stage->create_info, Context.Allocator);
	ASSERT(shader_stage->shader_module);

	// Release the resource.
	ResourceSystem::Unload(&BinaryResource);

	// Shader stage info.
	Memory::Zero(&shader_stage->shader_stage_create_info, sizeof(vk::ShaderModuleCreateInfo));
	shader_stage->shader_stage_create_info.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
	shader_stage->shader_stage_create_info.stage = config.stage;
	shader_stage->shader_stage_create_info.module = shader_stage->shader_module;
	shader_stage->shader_stage_create_info.pName = "main";

	return true;
}

IRenderpass* VulkanBackend::GetRenderpass(const char* name) {
	if (name == nullptr || name[0] == '\0') {
		LOG_ERROR("VulkanBackend::GetRenderpass() requires a name. nullptr will be returned.");
		return nullptr;
	}

	uint32_t ID = INVALID_ID;
	Context.RenderpassTable.Get(name, &ID);
	if (ID == INVALID_ID) {
		LOG_WARN("There is no registered renderpass named '%s'.", name);
		return nullptr;
	}

	return &Context.RegisteredPasses[ID];
}

void VulkanBackend::CreateRenderTarget(unsigned char attachment_count, std::vector<Texture*> attachments, IRenderpass* pass, uint32_t width, uint32_t height, RenderTarget* out_target) {
	// Max number of attachments.
	vk::ImageView AttachmentViews[32];
	for (uint32_t i = 0; i < attachment_count; ++i) {
		AttachmentViews[i] = ((VulkanImage*)attachments[i]->InternalData)->ImageView;
	}

	// Take a copy of the attachments and count.
	out_target->attachment_count = attachment_count;
	if (out_target->attachments.empty()) {
		out_target->attachments.resize(attachment_count);
	}
	for (const auto& att : attachments) {
		out_target->attachments.push_back(att);
	}

	vk::FramebufferCreateInfo FramebufferCreateInfo;
	FramebufferCreateInfo.setRenderPass(((VulkanRenderPass*)pass)->GetRenderPass())
		.setAttachmentCount(attachment_count)
		.setPAttachments(AttachmentViews)
		.setWidth(width)
		.setHeight(height)
		.setLayers(1);

	if (Context.Device.GetLogicalDevice().createFramebuffer(&FramebufferCreateInfo, Context.Allocator,
		(vk::Framebuffer*)&out_target->internal_framebuffer) != vk::Result::eSuccess) {
		LOG_ERROR("VulkanBackend::CreateRenderTarget() Failed to create framebuffers.");
	}
}

void  VulkanBackend::DestroyRenderTarget(RenderTarget* target, bool free_internal_memory) {
	if (target && target->internal_framebuffer) {
		Context.Device.GetLogicalDevice().destroyFramebuffer(*(vk::Framebuffer*)&target->internal_framebuffer, Context.Allocator);
		target->internal_framebuffer = nullptr;
		if (free_internal_memory) {
			target->attachments.clear();
			target->attachment_count = 0;
		}
	}
}

Texture* VulkanBackend::GetWindowAttachment(unsigned char index) {
	if (index >= Context.Swapchain.ImageCount) {
		LOG_FATAL("Attempting to get attachment index out of range: %d. Attachment count: %d.", index, Context.Swapchain.ImageCount);
		return nullptr;
	}

	return Context.Swapchain.RenderTextures[index];
}

Texture* VulkanBackend::GetDepthAttachment() {
	return Context.Swapchain.DepthTexture;
}
unsigned char VulkanBackend::GetWindowAttachmentIndex() {
	return (unsigned char)Context.ImageIndex;
}

void VulkanBackend::CreateRenderpass(IRenderpass* out_renderpass, float depth, uint32_t stencil, bool has_prev_pass, bool has_next_pass) {
	out_renderpass->Create(&Context, depth, stencil, has_prev_pass, has_next_pass);
}

void VulkanBackend::DestroyRenderpass(IRenderpass* pass) {
	pass->Destroy(&Context);
}

bool VulkanBackend::GetEnabledMultiThread() const {
	return Context.EnableMultithreading;
}

bool VulkanBackend::CreateRenderbuffer(enum RenderbufferType type, size_t total_size, bool use_freelist, IRenderbuffer* buffer) {
	if (buffer == nullptr) {
		buffer = new VulkanBuffer();
	}

	VulkanBuffer* VBuffer = (VulkanBuffer*)buffer;

	VBuffer->TotalSize = total_size;
	VBuffer->Type = type;
	VBuffer->UseFreelist = use_freelist;

	// Create freelist if needed.
	if (use_freelist) {
		VBuffer->BufferFreelist.Create(total_size);
	}

	// Create actual instance from the backend.
	if (!CreateRenderbuffer(buffer)) {
		LOG_FATAL("Unable to create backing buffer for renderbuffer, Application cannot continue.");
		return false;
	}

	return true;
}


bool VulkanBackend::AllocateRenderbuffer(IRenderbuffer* buffer, size_t size, size_t* out_offset) {
	if (buffer == nullptr || size == 0 || out_offset == nullptr) {
		LOG_ERROR("IRenderer::AllocateRenderbuffer() Requires valid pointer, a non-zero size and valid pointer to hlod offset.");
		return false;
	}

	if (!buffer->UseFreelist) {
		LOG_WARN("IRenderer::AllocateRenderbuffer() Called on a buffer not using freelist. Offset will not be valid. Call LoadData() instead.");
		*out_offset = 0;
		return true;
	}

	return buffer->BufferFreelist.AllocateBlock(size, out_offset);
}

bool VulkanBackend::FreeRenderbuffer(IRenderbuffer* buffer, size_t size, size_t offset) {
	if (buffer == nullptr || size == 0) {
		LOG_ERROR("IRenderer::AllocateRenderbuffer() Requires valid pointer, a non-zero size.");
		return false;
	}

	if (!buffer->UseFreelist) {
		LOG_WARN("IRenderer::AllocateRenderbuffer() Called on a buffer not using freelist. Nothing was down.");
		return true;
	}

	return buffer->BufferFreelist.FreeBlock(size, offset);
}

bool VulkanBackend::CreateRenderbuffer(IRenderbuffer* buffer) {
	return ((VulkanBuffer*)buffer)->Create(&Context);
}

void VulkanBackend::DestroyRenderbuffer(IRenderbuffer* buffer) {
	if (buffer == nullptr) {
		return;
	}

	if (buffer->UseFreelist) {
		buffer->BufferFreelist.Destroy();
	}

	((VulkanBuffer*)buffer)->Destroy(&Context);
}

bool VulkanBackend::ReadRenderbuffer(IRenderbuffer* buffer, size_t offset, size_t size, void** out_memory) {
	vk::Device LogicalDevice = Context.Device.GetLogicalDevice();
	VulkanBuffer* VBuffer = (VulkanBuffer*)buffer;
	if (VBuffer->IsDeviceLocal() && !VBuffer->IsHostVisible()) {
		// NOTE: If a read buffer is needed (i.e.) the target buffer's memory is not host visible but is device-local,
		// create the read buffer, copy data to it, then read from that buffer.

		// Create a host-visible staging buffer to copy to. Mark it as the destination of the transfer.
		VulkanBuffer Read;
		if (!CreateRenderbuffer(RenderbufferType::eRenderbuffer_Type_Read, size, false, &Read)) {
			LOG_ERROR("VulkanBackend::ReadRenderbuffer() Failed to create read buffer.");
			return false;
		}

		BindRenderbuffer(&Read, 0);
		
		// Perform the copy from device local to the read buffer.
		CopyRange(VBuffer, offset, &Read, 0, size);

		// Map/copy/unmap
		void* MappedData = nullptr;
		MappedData = Read.MapMemory(&Context, 0, size);
		Memory::Copy(*out_memory, MappedData, size);
		Read.UnmapMemory(&Context);

		// Clean up the read buffer.
		UnBindRenderbuffer(&Read);
		DestroyRenderbuffer(&Read);
	}
	else {
		// If no staging buffer is needed, map/copy/unmap.
		void* DataPtr;
		if (LogicalDevice.mapMemory(((VulkanBuffer*)buffer)->Memory, offset, size, vk::MemoryMapFlags(), &DataPtr) != vk::Result::eSuccess) {
			LOG_ERROR("Map memory Failed.");
			return false;
		}

		Memory::Copy(*out_memory, DataPtr, size);
		LogicalDevice.unmapMemory(((VulkanBuffer*)buffer)->Memory);
	}

	return true;
}
bool VulkanBackend::ResizeRenderbuffer(IRenderbuffer* buffer, size_t new_size) {
	VulkanBuffer* VBuffer = (VulkanBuffer*)buffer;

	// Sanity check.
	if (new_size < VBuffer->TotalSize) {
		LOG_ERROR("IRenderer::ResizeRenderbuffer() Failed to resize renderbuffer. Can not resize a smaller buffer.");
		return false;
	}

	if (VBuffer->UseFreelist) {
		// Resize the freelist first if used.
		if (!VBuffer->BufferFreelist.Resize(new_size)) {
			LOG_ERROR("Failed to resize free list.");
			return false;
		}
	}

	bool Result = VBuffer->Resize(&Context, new_size);
	if (Result) {
		VBuffer->TotalSize = new_size;
	}

	return Result;
}

bool VulkanBackend::LoadRange(IRenderbuffer* buffer, size_t offset, size_t size, const void* data) {
	VulkanBuffer* VBuffer = (VulkanBuffer*)buffer;
	if (VBuffer->IsDeviceLocal() && !VBuffer->IsHostVisible()) {
		// NOTE: If a staging buffer is needed (i.e.) the target buffer's memory is not host visible but is device-local,
		// create a staging buffer to load the data into first. Then copy from it to the target buffer.

		// Create a host-visible staging buffer to upload to. Mark it as the source of the transfer.
		VulkanBuffer Staging;
		if (!CreateRenderbuffer(RenderbufferType::eRenderbuffer_Type_Staging, size, false, &Staging)) {
			LOG_ERROR("VulkanBackend::ReadRenderbuffer() Failed to create staging buffer.");
			return false;
		}

		BindRenderbuffer(&Staging, 0);

		// Load the data into the staging buffer.
		LoadRange(&Staging, 0, size, data);

		// Perform the copy from staging to the device local buffer.
		CopyRange(&Staging, 0, VBuffer, offset, size);

		// Clean up the staging buffer.
		UnBindRenderbuffer(&Staging);
		DestroyRenderbuffer(&Staging);
	}
	else {
		// If no staging buffer is needed, map/copy/unmap.
		void* DataPtr;
		if (Context.Device.GetLogicalDevice().mapMemory(((VulkanBuffer*)buffer)->Memory, offset, size, vk::MemoryMapFlags(), &DataPtr) != vk::Result::eSuccess) {
			LOG_ERROR("Map memory Failed.");
			return false;
		}
		Memory::Copy(DataPtr, data, size);
		Context.Device.GetLogicalDevice().unmapMemory(((VulkanBuffer*)buffer)->Memory);
	}

	return true;
}

bool VulkanBackend::CopyRange(IRenderbuffer* src, size_t src_offset, IRenderbuffer* dst, size_t dst_offset, size_t size) {
	return dst->CopyRange(&Context, ((VulkanBuffer*)src)->Buffer, src_offset, ((VulkanBuffer*)dst)->Buffer, dst_offset, size);
}

bool VulkanBackend::DrawRenderbuffer(IRenderbuffer* buffer, size_t offset, uint32_t element_count, bool bind_only) {
	VulkanCommandBuffer* CmdBuffer = &Context.GraphicsCommandBuffers[Context.ImageIndex];

	if (buffer->Type == RenderbufferType::eRenderbuffer_Type_Vertex) {
		// Bind vertex buffer at offset.
		vk::DeviceSize Offsets[1] = { offset };
		CmdBuffer->CommandBuffer.bindVertexBuffers(0, 1, &((VulkanBuffer*)buffer)->Buffer, Offsets);
		if (!bind_only) {
			CmdBuffer->CommandBuffer.draw(element_count, 1, 0, 0);
		}
		return true;
	}
	else if (buffer->Type == RenderbufferType::eRenderbuffer_Type_Index) {
		// Bind index buffer at offset.
		CmdBuffer->CommandBuffer.bindIndexBuffer(((VulkanBuffer*)buffer)->Buffer, offset, vk::IndexType::eUint32);
		if (!bind_only) {
			CmdBuffer->CommandBuffer.drawIndexed(element_count, 1, 0, 0, 0);
		}
		return true;
	}
	else {
		LOG_ERROR("Can not draw buffer of type: %i.", buffer->Type);
	}

	return false;
}

bool VulkanBackend::BindRenderbuffer(IRenderbuffer* buffer, size_t offset) {
	return ((VulkanBuffer*)buffer)->Bind(&Context, offset);
}

bool VulkanBackend::UnBindRenderbuffer(IRenderbuffer* buffer) {
	return ((VulkanBuffer*)buffer)->UnBind(&Context);
}

void* VulkanBackend::MapMemory(IRenderbuffer* buffer, size_t offset, size_t size) {
	return ((VulkanBuffer*)buffer)->MapMemory(&Context, offset, size);
}

void VulkanBackend::UnmapMemory(IRenderbuffer* buffer, size_t offset, size_t size) {
	((VulkanBuffer*)buffer)->UnmapMemory(&Context);
}

bool VulkanBackend::FlushRenderbuffer(IRenderbuffer* buffer, size_t offset, size_t size) {
	return ((VulkanBuffer*)buffer)->Flush(&Context, offset, size);
}