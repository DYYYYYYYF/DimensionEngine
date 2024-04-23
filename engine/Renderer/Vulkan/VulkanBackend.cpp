#include "VulkanBackend.hpp"
#include "VulkanPlatform.hpp"
#include "VulkanDevice.hpp"

#include "Core/Application.hpp"
#include "Core/EngineLogger.hpp"
#include "Containers/TArray.hpp"
#include "Platform/Platform.hpp"

static uint32_t CachedFramebufferWidth = 0;
static uint32_t CachedFramebufferHeight = 0;

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT message_servity,
	VkDebugUtilsMessageTypeFlagsEXT message_type,
	const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
	void* user_data
);

VulkanBackend::VulkanBackend() {
}

VulkanBackend::~VulkanBackend() {

}

bool VulkanBackend::Initialize(const char* application_name, struct SPlatformState* plat_state) {

	// TODO: Custom allocator;
	Context.Allocator = nullptr;

	GetFramebufferSize(&CachedFramebufferWidth, &CachedFramebufferHeight);
	Context.FrameBufferWidth = (CachedFramebufferWidth > 0) ? CachedFramebufferWidth : 800;
	Context.FrameBufferHeight = (CachedFramebufferHeight > 0) ? CachedFramebufferHeight : 600;
	CachedFramebufferWidth = 0;
	CachedFramebufferHeight = 0;

	vk::ApplicationInfo ApplicationInfo;
	vk::InstanceCreateInfo InstanceInfo;

	ApplicationInfo.setApiVersion(VK_API_VERSION_1_3)
		.setApplicationVersion((1, 0, 0))
		.setPApplicationName(application_name)
		.setEngineVersion((1, 0, 0))
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
	UL_DEBUG("%s", output.c_str());
#endif
	InstanceInfo.setEnabledExtensionCount((uint32_t)RequiredExtensions.size())
		.setPEnabledExtensionNames(RequiredExtensions);

	// Validation layers
	std::vector<const char*> RequiredValidationLayerName;

#ifdef LEVEL_DEBUG
	UL_INFO("Validation layers enabled. Enumerating ...");

	// List of validation layers required
	RequiredValidationLayerName.push_back("VK_LAYER_KHRONOS_validation");

	// Obtain a list of available validation layers
	uint32_t AvailableLayersCount = 0;
	std::vector<vk::LayerProperties> AvailableLayers;
	vk::Result result;
	result = vk::enumerateInstanceLayerProperties(&AvailableLayersCount, AvailableLayers.data());
	if (result != vk::Result::eSuccess) {
		UL_FATAL("Enum instance layer properties failed.");
		return false;
	}
	AvailableLayers.resize(AvailableLayersCount);
	result = vk::enumerateInstanceLayerProperties(&AvailableLayersCount, AvailableLayers.data());
	ASSERT(result == vk::Result::eSuccess);

	// Verify all required layers are available.
	for (uint32_t i = 0; i < RequiredValidationLayerName.size(); i++) {
		UL_INFO("Searching for layer: %s...", RequiredValidationLayerName[i]);
		
		bool IsFound = false;
		for (uint32_t j = 0; j < AvailableLayersCount; j++) {
			if (strcmp(RequiredValidationLayerName[i], AvailableLayers[j].layerName)) {
				IsFound = true;
				UL_INFO("Found.");
				break;
			}
		}

		if (!IsFound) {
			UL_FATAL("Required validation layer is missing: %s", RequiredValidationLayerName[i]);
			return false;
		}
	}

	UL_INFO("All required validation layers are present.");
#endif
	InstanceInfo.setEnabledLayerCount(0)
		.setPEnabledLayerNames(nullptr);

	try
	{
		Context.Instance = vk::createInstance(InstanceInfo, Context.Allocator);
	}
	catch (const std::exception&)
	{
		UL_ERROR("Create vulkan instance failed.");
		return false;
	}
	ASSERT(Context.Instance);

#ifdef LEVEL_DEBUG
	UL_INFO("Create vulkan debugger...");
	
	vk::DebugUtilsMessageSeverityFlagsEXT LogServerity = 
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;
	// | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
	// | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;

	vk::DebugUtilsMessageTypeFlagsEXT MessageType =
		vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding |
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
		UL_FATAL("Create debug utils messenger failed.");
		return false;
	}

	UL_INFO("Vulkan debugger created.");
#endif

	// Surface
	UL_INFO("Creating vulkan surface...");
	if (!PlatformCreateVulkanSurface(plat_state, &Context)) {
		UL_ERROR("Create platform surface failed.");
		return false;
	}
	UL_INFO("Vulkan surface created.");

	// Device
	UL_INFO("Creating vulkan device...");
	if (!Context.Device.Create(&Context.Instance, Context.Allocator, Context.Surface)) {
		UL_ERROR("Create vulkan device failed.");
		return false;
	}
	UL_INFO("Vulkan device created.");

	// Swapchain
	Context.Swapchain.Create(&Context, Context.FrameBufferWidth, Context.FrameBufferHeight);

	Context.MainRenderPass.Create(&Context, 0.0f, 0.0f, (float)Context.FrameBufferWidth, (float)Context.FrameBufferHeight, 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 0);
	
	// Swapchain framebuffers.
	Context.Swapchain.Framebuffers.resize(Context.Swapchain.ImageCount);
	RegenerateFrameBuffers();

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
		Context.InFlightFences[i].Create(&Context, true);
	}

	// In flight fences should not yet exist at this point, so clear the list. These are stored in pointers
	// because the initial state should be 0, and will be 0 when not in use. Actual fences are not owned by this list
	Context.ImagesInFilght.resize(Context.Swapchain.ImageCount);
	for (uint32_t i = 0; i < Context.Swapchain.ImageCount; ++i) {
		Context.ImagesInFilght[i] = nullptr;
	}


	UL_INFO("Create vulkan instance succeed.");
	return true;
}

void VulkanBackend::Shutdown() {
	vk::Device LogicalDevice = Context.Device.GetLogicalDevice();
	LogicalDevice.waitIdle();

	UL_DEBUG("Destroying sync objects.");
	for (uint32_t i = 0; i < Context.Swapchain.MaxFramesInFlight; ++i) {
		if (Context.ImageAvailableSemaphores[i]) {
			LogicalDevice.destroySemaphore(Context.ImageAvailableSemaphores[i], Context.Allocator);
		}
		if (Context.QueueCompleteSemaphores[i]) {
			LogicalDevice.destroySemaphore(Context.QueueCompleteSemaphores[i], Context.Allocator);
		}

		Context.InFlightFences[i].Destroy(&Context);
	}

	Context.ImageAvailableSemaphores.clear();
	Context.QueueCompleteSemaphores.clear();
	Context.InFlightFences.clear();
	Context.ImagesInFilght.clear();

	UL_DEBUG("Destroying command buffers.");
	for (uint32_t i = 0; i < Context.Swapchain.ImageCount; ++i) {
		if (Context.GraphicsCommandBuffers[i].CommandBuffer) {
			Context.GraphicsCommandBuffers[i].Free(&Context, Context.Device.GetGraphicsCommandPool());
		}
	}

	UL_DEBUG("Destroying command buffers.");
	for (uint32_t i = 0; i < Context.Swapchain.ImageCount; ++i) {
		Context.Swapchain.Framebuffers[i].Destroy(&Context);
	}

	UL_DEBUG("Destroying render pass.");
	Context.MainRenderPass.Destroy(&Context);

	UL_DEBUG("Destroying swapchain.");
	Context.Swapchain.Destroy(&Context);

	UL_DEBUG("Destroying vulkan device.");
	Context.Device.Destroy(&Context.Instance);

	UL_DEBUG("Destroying vulkan surface.");
	if (Context.Surface) {
		Context.Instance.destroy(Context.Surface, Context.Allocator);
	}

#ifdef LEVEL_DEBUG
	UL_DEBUG("Destroying vulkan debugger...");
	auto dispatcher = vk::DispatchLoaderDynamic(Context.Instance, vkGetInstanceProcAddr);
	if (Context.DebugMessenger) {
		Context.Instance.destroyDebugUtilsMessengerEXT(Context.DebugMessenger, Context.Allocator, dispatcher);
	}
#endif

	UL_DEBUG("Destroying vulkan instance...");
	Context.Instance.destroy(Context.Allocator);

}

bool VulkanBackend::BeginFrame(double delta_time){

	return true;
}

bool VulkanBackend::EndFrame(double delta_time) {

	return true;
}

void VulkanBackend::Resize(unsigned short width, unsigned short height) {

}

void VulkanBackend::CreateCommandBuffer() {
	if (Context.GraphicsCommandBuffers.empty()) {
		Context.GraphicsCommandBuffers.resize(Context.Swapchain.ImageCount);
		for (uint32_t i = 0; i < Context.Swapchain.ImageCount; ++i) {
			Memory::Zero(&(Context.GraphicsCommandBuffers[i]), sizeof(vk::CommandBuffer));
		}
	}

	ASSERT(Context.Swapchain.ImageCount == Context.GraphicsCommandBuffers.size());
	for (uint32_t i = 0; i < Context.Swapchain.ImageCount; ++i) {
		if (Context.GraphicsCommandBuffers[i].CommandBuffer) {
			Context.GraphicsCommandBuffers[i].Free(&Context, Context.Device.GetGraphicsCommandPool());
		}

		Memory::Zero(&Context.GraphicsCommandBuffers[i], sizeof(vk::CommandBuffer));
		Context.GraphicsCommandBuffers[i].Allocate(&Context, Context.Device.GetGraphicsCommandPool(), true);
	}

	UL_INFO("Vulkan command buffers created.");
}

void VulkanBackend::RegenerateFrameBuffers() {
	for (uint32_t i = 0; i < Context.Swapchain.ImageCount; ++i) {
		// TODO: Make this dynamic based on the currently configured attachments
		uint32_t AttachmentCount = 2;
		vk::ImageView Attachments[] = {
			Context.Swapchain.ImageViews[i],
			Context.Swapchain.DepthAttachment.ImageView
		};

		Context.Swapchain.Framebuffers[i].Create(&Context, &Context.MainRenderPass, 
			Context.FrameBufferWidth, Context.FrameBufferHeight, AttachmentCount, Attachments);
	}
}

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
		UL_ERROR(callback_data->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		UL_WARN(callback_data->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		UL_FATAL(callback_data->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		UL_INFO(callback_data->pMessage);
		break;
	}

	return VK_FALSE;
}