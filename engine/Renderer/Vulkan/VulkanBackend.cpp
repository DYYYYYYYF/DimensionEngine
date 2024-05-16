#include "VulkanBackend.hpp"
#include "VulkanPlatform.hpp"
#include "VulkanDevice.hpp"

#include "Core/Application.hpp"
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

static uint32_t CachedFramebufferWidth = 0;
static uint32_t CachedFramebufferHeight = 0;

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
			if (strcmp(RequiredValidationLayerName[i], AvailableLayers[j].layerName.data()) == 0) {
				IsFound = true;
				UL_INFO("Found.");
				break;
			}
		}

		if (!IsFound) {
			UL_WARN("Required validation layer is missing: '%s'!", RequiredValidationLayerName[i]);

			// TODO: Remove spec-element.
			RequiredValidationLayerName.clear();
		}
	}

#endif
	InstanceInfo.setEnabledLayerCount((uint32_t)RequiredValidationLayerName.size())
		.setPEnabledLayerNames(RequiredValidationLayerName);

	Context.Instance = vk::createInstance(InstanceInfo, Context.Allocator);
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

	// World renderpass
	Context.MainRenderPass.Create(&Context, 
		Vec4(0.0f, 0.0f, (float)Context.FrameBufferWidth, (float)Context.FrameBufferHeight), 
		Vec4(1.0f, 0.5f, 0.5f, 1.0f), 1.0f, 0, 
		eRenderpass_Clear_Color_Buffer | eRenderpass_Clear_Depth_Buffer | eRenderpass_Clear_Stencil_Buffer,
		false, true);
	
	// UI renderpass
	Context.UIRenderPass.Create(&Context,
		Vec4(0.0f, 0.0f, (float)Context.FrameBufferWidth, (float)Context.FrameBufferHeight),
		Vec4(0.0f, 0.0f, 0.0f, 0.0f), 1.0f, 0,
		eRenderpass_Clear_None,
		true, false);

	// Swapchain framebuffers.
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

	CreateBuffers();

	// Mark all geometry as invalid.
	for (uint32_t i = 0; i < GEOMETRY_MAX_COUNT; ++i) {
		Context.Geometries[i].id = INVALID_ID;
	}

	UL_INFO("Create vulkan instance succeed.");
	return true;
}

void VulkanBackend::Shutdown() {
	vk::Device LogicalDevice = Context.Device.GetLogicalDevice();
	LogicalDevice.waitIdle();

	UL_DEBUG("Destroying Buffers");
	Context.ObjectVertexBuffer.Destroy(&Context);
	Context.ObjectIndexBuffer.Destroy(&Context);

	UL_DEBUG("Destroying sync objects.");
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

	UL_DEBUG("Destroying command buffers.");
	for (uint32_t i = 0; i < Context.Swapchain.ImageCount; ++i) {
		if (Context.GraphicsCommandBuffers[i].CommandBuffer) {
			Context.GraphicsCommandBuffers[i].Free(&Context, Context.Device.GetGraphicsCommandPool());
		}
	}

	UL_DEBUG("Destroying frame buffers.");
	for (uint32_t i = 0; i < Context.Swapchain.ImageCount; ++i) {
		Context.Device.GetLogicalDevice().destroyFramebuffer(Context.WorldFramebuffers[i], Context.Allocator);
		Context.Device.GetLogicalDevice().destroyFramebuffer(Context.Swapchain.Framebuffers[i], Context.Allocator);
	}

	UL_DEBUG("Destroying render pass ui.");
	Context.UIRenderPass.Destroy(&Context);
	UL_DEBUG("Destroying render pass world.");
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
	Context.FrameDeltaTime = delta_time;
	VulkanDevice* Device = &Context.Device;

	// Check if recreating swap chain and boot out
	if (Context.RecreatingSwapchain) {
		Device->GetLogicalDevice().waitIdle();
		UL_INFO("Recreating swapchain, booting.");
		return false;
	}

	// Check if the framebuffer has been resized. If so, a new swapchain must be created.
	if (Context.FramebufferSizeGenerate != Context.FramebufferSizeGenerateLast) {
		Device->GetLogicalDevice().waitIdle();

		// If the swap chain recreation failed
		// boot out before unsetting the flag
		if (!RecreateSwapchain()) {
			UL_INFO("Recreating swapchain, booting.");
			return false;
		}

		UL_INFO("Resized, booting.");
		return false;
	}

	// Wait for the execution of the current frame to complete. The fence being free will allow this one to move on
	if (Context.Device.GetLogicalDevice().waitForFences(1, &Context.InFlightFences[Context.CurrentFrame], true, UINT64_MAX)
		!= vk::Result::eSuccess) {
		UL_WARN("In flight fence wait failure!");
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

	Context.MainRenderPass.SetW((float)Context.FrameBufferWidth);
	Context.MainRenderPass.SetH((float)Context.FrameBufferHeight);

	return true;
}

bool VulkanBackend::EndFrame(double delta_time) {

	VulkanCommandBuffer* CommandBuffer = &Context.GraphicsCommandBuffers[Context.ImageIndex];
	
	CommandBuffer->EndCommand();

	// Make sure the previous frame is not using this image
	if (Context.ImagesInFilght[Context.ImageIndex] != VK_NULL_HANDLE) {
		if (Context.Device.GetLogicalDevice().waitForFences(1, Context.ImagesInFilght[Context.ImageIndex], true, UINT64_MAX)
			!= vk::Result::eSuccess) {
			UL_WARN("In flight fence wait failure!");
			return false;
		}
	}

	// Make sure image fence as in use by this frame
	Context.ImagesInFilght[Context.ImageIndex] = &Context.InFlightFences[Context.CurrentFrame];

	// Reset the fence for use on the next frame
	if (Context.Device.GetLogicalDevice().resetFences(1, &Context.InFlightFences[Context.CurrentFrame])
		!= vk::Result::eSuccess) {
		UL_WARN("In flight fence wait failure!");
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
	vk::PipelineStageFlags Flags[1] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
	SubmitInfo.setWaitDstStageMask(Flags);

	if (Context.Device.GetGraphicsQueue().submit(1, &SubmitInfo, Context.InFlightFences[Context.CurrentFrame]) != vk::Result::eSuccess) {
		UL_ERROR("Queue submit failed.");
		return false;
	}

	CommandBuffer->UpdateSubmitted();

	// End queue submission
	Context.Swapchain.Presnet(&Context, Context.Device.GetGraphicsQueue(), Context.Device.GetPresentQueue(),
		Context.QueueCompleteSemaphores[Context.CurrentFrame], Context.ImageIndex);

	return true;
}

void VulkanBackend::Resize(unsigned short width, unsigned short height) {
	// Update the "Framebuffer size generate", a counter which indicates when the
	// framebuffer size has been updated
	CachedFramebufferWidth = width;
	CachedFramebufferHeight = height;
	Context.FramebufferSizeGenerate++;

	UL_INFO("Vulkan renderer backend resize: width/height/generation: %i/%i/%llu", width, height, Context.FramebufferSizeGenerate);
}

void VulkanBackend::CreateCommandBuffer() {
	if (Context.GraphicsCommandBuffers == nullptr) {
		Context.GraphicsCommandBuffers = (VulkanCommandBuffer*)Memory::Allocate(sizeof(VulkanCommandBuffer) * Context.Swapchain.ImageCount, MemoryType::eMemory_Type_Renderer);
		for (uint32_t i = 0; i < Context.Swapchain.ImageCount; ++i) {
			Memory::Zero(&(Context.GraphicsCommandBuffers[i]), sizeof(vk::CommandBuffer));
		}
	}

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
	uint32_t ImageCount = Context.Swapchain.ImageCount;
	for (uint32_t i = 0; i < ImageCount; ++i) {
		uint32_t AttachmentCount = 2;
		vk::ImageView WorldAttachments[2] = {
			Context.Swapchain.ImageViews[i],
			Context.Swapchain.DepthAttachment.ImageView
		};

		vk::FramebufferCreateInfo FramebufferCreateInfo;
		FramebufferCreateInfo.setRenderPass(Context.MainRenderPass.GetRenderPass())
			.setAttachmentCount(AttachmentCount)
			.setPAttachments(WorldAttachments)
			.setWidth(Context.FrameBufferWidth)
			.setHeight(Context.FrameBufferHeight)
			.setLayers(1);

		Context.WorldFramebuffers[i] = Context.Device.GetLogicalDevice().createFramebuffer(FramebufferCreateInfo, Context.Allocator);
		ASSERT(Context.WorldFramebuffers[i]);

		// Swapchain framebuffers (UI pass). Outputs to swapchain image.
		uint32_t UIAttachmentCount = 1;
		vk::ImageView UIAttachments[1] = {
			Context.Swapchain.ImageViews[i],
		};

		vk::FramebufferCreateInfo SCFramebufferCreateInfo;
		SCFramebufferCreateInfo.setRenderPass(Context.UIRenderPass.GetRenderPass())
			.setAttachmentCount(UIAttachmentCount)
			.setPAttachments(UIAttachments)
			.setWidth(Context.FrameBufferWidth)
			.setHeight(Context.FrameBufferHeight)
			.setLayers(1);

		Context.Swapchain.Framebuffers[i] = Context.Device.GetLogicalDevice().createFramebuffer(SCFramebufferCreateInfo, Context.Allocator);
		ASSERT(Context.Swapchain.Framebuffers[i]);
	}
}

bool VulkanBackend::RecreateSwapchain() {
	// If already being recreated, do not try again.
	if (Context.RecreatingSwapchain) {
		UL_DEBUG("Already called recreate swapchain. Booting.");
		return false;
	}
	
	// Detect if the windows is too small to be drawn to
	if (Context.FrameBufferWidth == 0 || Context.FrameBufferHeight == 0) {
		UL_DEBUG("Recreate swapchain called when windows is < 1px. Booting.");
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

	Context.Swapchain.Recreate(&Context, CachedFramebufferWidth, CachedFramebufferHeight);

	// Sync the framebuffer size with the cached sizes.
	Context.FrameBufferWidth = CachedFramebufferWidth;
	Context.FrameBufferHeight = CachedFramebufferHeight;
	Context.MainRenderPass.SetW((float)Context.FrameBufferWidth);
	Context.MainRenderPass.SetH((float)Context.FrameBufferHeight);
	Context.UIRenderPass.SetW((float)Context.FrameBufferWidth);
	Context.UIRenderPass.SetH((float)Context.FrameBufferHeight);
	CachedFramebufferWidth = 0;
	CachedFramebufferHeight = 0;

	// Update framebuffer size generation.
	Context.FramebufferSizeGenerateLast = Context.FramebufferSizeGenerate;

	// Cleanup swapcahin
	for (uint32_t i = 0; i < Context.Swapchain.ImageCount; ++i) {
		Context.GraphicsCommandBuffers[i].Free(&Context, Context.Device.GetGraphicsCommandPool());
	}

	// Framebuffers
	for (uint32_t i = 0; i < Context.Swapchain.ImageCount; ++i) {
		Context.Device.GetLogicalDevice().destroyFramebuffer(Context.WorldFramebuffers[i], Context.Allocator);
		Context.Device.GetLogicalDevice().destroyFramebuffer(Context.Swapchain.Framebuffers[i], Context.Allocator);
	}

	Context.MainRenderPass.SetX(0);
	Context.MainRenderPass.SetY(0);
	Context.MainRenderPass.SetW((float)Context.FrameBufferWidth);
	Context.MainRenderPass.SetH((float)Context.FrameBufferHeight);
	Context.UIRenderPass.SetX(0);
	Context.UIRenderPass.SetY(0);
	Context.UIRenderPass.SetW((float)Context.FrameBufferWidth);
	Context.UIRenderPass.SetH((float)Context.FrameBufferHeight);

	RegenerateFrameBuffers();
	CreateCommandBuffer();

	// Clear the recreating flag.
	Context.RecreatingSwapchain = false;

	return true;
}

bool VulkanBackend::CreateBuffers() {
	vk::MemoryPropertyFlagBits MemoryPropertyFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;

	// Geometry vertex buffer
	const size_t VertexBufferSize = sizeof(Vertex) * 1024 * 1024;
	if (!Context.ObjectVertexBuffer.Create(&Context, VertexBufferSize, 
		vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc,
		MemoryPropertyFlags, true, true)) {
		UL_ERROR("Error creating vertex buffer.");
		return false;
	}

	// Geometry index buffer
	const size_t IndexBufferSize = sizeof(uint32_t) * 1024 * 1024;
	if (!Context.ObjectIndexBuffer.Create(&Context, IndexBufferSize,
		vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc,
		MemoryPropertyFlags, true, true)) {
		UL_ERROR("Error creating index buffer.");
		return false;
	}

	return true;
}

bool VulkanBackend::UploadDataRange(vk::CommandPool pool, vk::Fence fence, vk::Queue queue, VulkanBuffer* buffer, size_t* offset, size_t size, const void* data) {
	// Allocate space in the buffer.
	if (!buffer->Allocate(size, offset)) {
		UL_ERROR("Upload data range failed to allocate from the buffer.");
		return false;
	}

	// Create a host-visible staging buffer to upload to. Mark it as the source of the transfer.
	vk::MemoryPropertyFlags Flags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
	VulkanBuffer Staging;
	Staging.Create(&Context, size, vk::BufferUsageFlagBits::eTransferSrc, Flags, true, false);

	// Load the data into the staging buffer.
	Staging.LoadData(&Context, 0, size, vk::MemoryMapFlags(), data);

	// Perform the copy from staging to the device local buffer.
	Staging.CopyTo(&Context, pool, fence, queue, 
		Staging.Buffer, 0 , buffer->Buffer, *offset, size);

	// Clean up the staging buffer.
	Staging.Destroy(&Context);

	return true;
}

void VulkanBackend::FreeDataRange(VulkanBuffer* buffer, size_t offset, size_t size) {
	if (buffer) {
		buffer->Free(size, offset);
	}
}

void VulkanBackend::CreateTexture(const unsigned char* pixels, Texture* texture) {
	// Internal data creation.
	// TODO: Use an allocator for this.
	texture->InternalData = (VulkanTexture*)Memory::Allocate(sizeof(VulkanTexture), MemoryType::eMemory_Type_Texture);
	VulkanTexture* Data = (VulkanTexture*)texture->InternalData;
	vk::DeviceSize ImageSize = texture->Width * texture->Height * texture->ChannelCount;

	// NOTE: Assumes 8 bits per channel.
	vk::Format ImageFormat = vk::Format::eR8G8B8A8Unorm;

	// Create a staging buffer and load data into it.
	vk::BufferUsageFlags Usage = vk::BufferUsageFlagBits::eTransferSrc;
	vk::MemoryPropertyFlags MemoryPropFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
	VulkanBuffer Staging;
	Staging.Create(&Context, ImageSize, Usage, MemoryPropFlags, true, false);

	Staging.LoadData(&Context, 0, ImageSize, {}, pixels);

	// NOTE: Lots of assumptions here, different texture types will require.
	// different options here.
	Data->Image.CreateImage(&Context, vk::ImageType::e2D, texture->Width, texture->Height, ImageFormat,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst |
		vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment,
		vk::MemoryPropertyFlagBits::eDeviceLocal,
		true, vk::ImageAspectFlagBits::eColor);

	VulkanCommandBuffer TempBuffer;
	vk::CommandPool Pool = Context.Device.GetGraphicsCommandPool();
	vk::Queue Queue = Context.Device.GetGraphicsQueue();
	TempBuffer.AllocateAndBeginSingleUse(&Context, Pool);

	// Transition the layout from whatever it is currently to optimal for receiving data.
	Data->Image.TransitionLayout(&Context, &TempBuffer, ImageFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

	// Copy the data from buffer.
	Data->Image.CopyFromBuffer(&Context, Staging.Buffer, &TempBuffer);

	// Transition from optimal for data receipt to shader-read-only optimal layout.
	Data->Image.TransitionLayout(&Context, &TempBuffer, ImageFormat, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

	TempBuffer.EndSingleUse(&Context, Pool, Queue);
	Staging.Destroy(&Context);

	// Create a sampler for the texture.
	vk::SamplerCreateInfo SamplerInfo;
	SamplerInfo.setMagFilter(vk::Filter::eLinear)
		.setMinFilter(vk::Filter::eLinear)
		.setAddressModeU(vk::SamplerAddressMode::eRepeat)
		.setAddressModeV(vk::SamplerAddressMode::eRepeat)
		.setAddressModeW(vk::SamplerAddressMode::eRepeat)
		.setAnisotropyEnable(VK_TRUE)
		.setMaxAnisotropy(16)
		.setBorderColor(vk::BorderColor::eIntOpaqueBlack)
		.setUnnormalizedCoordinates(VK_FALSE)
		.setCompareEnable(VK_FALSE)
		.setCompareOp(vk::CompareOp::eAlways)
		.setMipLodBias(0.0f)
		.setMipmapMode(vk::SamplerMipmapMode::eLinear)
		.setMinLod(0.0f)
		.setMaxLod(0.0f);

	Data->sampler = Context.Device.GetLogicalDevice().createSampler(SamplerInfo, Context.Allocator);
	ASSERT(Data->sampler);

	texture->Generation++;
}

void VulkanBackend::DestroyTexture(Texture* texture) {
	Context.Device.GetLogicalDevice().waitIdle();

	VulkanTexture* Data = (VulkanTexture*)texture->InternalData;

	if (Data != nullptr) {
		Data->Image.Destroy(&Context);
		Memory::Zero(&Data->Image, sizeof(VulkanImage));
		Context.Device.GetLogicalDevice().destroySampler(Data->sampler, Context.Allocator);
		Data->sampler = nullptr;

		Memory::Free(texture->InternalData, sizeof(VulkanTexture), MemoryType::eMemory_Type_Texture);
	}

	Memory::Zero(texture, sizeof(Texture));
}

bool VulkanBackend::CreateGeometry(Geometry* geometry, uint32_t vertex_size, uint32_t vertex_count, 
	const void* vertices, uint32_t index_size, uint32_t index_count, const void* indices) {
	if (vertex_count == 0 || vertices == nullptr) {
		UL_ERROR("Vulkan renderer create geometry requires vertex data, and none was supplied. vertex_count=%d, vertices=%p", vertex_count, vertices);
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
		UL_FATAL("Vulkan renderer create geometry failed to find a free index for a new geometry upload. Adjust config to allow for more.");
		return false;
	}

	vk::CommandPool Pool = Context.Device.GetGraphicsCommandPool();
	vk::Queue Queue = Context.Device.GetGraphicsQueue();

	// Vertex data.
	InternalData->vertex_count = vertex_count;
	InternalData->vertex_element_size = sizeof(Vertex);
	uint32_t VertexTotalSize = vertex_size * vertex_count;
	if (!UploadDataRange(Pool, nullptr, Queue, &Context.ObjectVertexBuffer,
		&InternalData->vertext_buffer_offset, VertexTotalSize, vertices)) {
		UL_ERROR("Vulkan renderer create geometry failed to upload vertex data.");
		return false;
	}

	// Index data. If Applicable.
	if (index_count != 0 && indices != nullptr) {
		InternalData->index_count = index_count;
		InternalData->index_element_size = sizeof(uint32_t);
		uint32_t IndexTotalSize = index_size * index_count;
		if (!UploadDataRange(Pool, nullptr, Queue, &Context.ObjectIndexBuffer,
			&InternalData->index_buffer_offset, IndexTotalSize, indices)) {
			UL_ERROR("Vulkan renderer create geometry failed to upload index data.");
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
		FreeDataRange(&Context.ObjectVertexBuffer, OldRange.vertext_buffer_offset, OldRange.vertex_element_size * OldRange.vertex_count);

		// Free index data.
		if (OldRange.index_element_size > 0) {
			FreeDataRange(&Context.ObjectIndexBuffer, OldRange.index_buffer_offset, OldRange.index_element_size * OldRange.index_count);
		}
	}

	return true;
}

void VulkanBackend::DestroyGeometry(Geometry* geometry) {
	if (geometry != nullptr && geometry->InternalID != INVALID_ID) {
		Context.Device.GetLogicalDevice().waitIdle();
		GeometryData* InternalData = &Context.Geometries[geometry->InternalID];

		// Free vertex data.
		FreeDataRange(&Context.ObjectVertexBuffer, InternalData->vertext_buffer_offset, InternalData->vertex_element_size * InternalData->vertex_count);

		// Free index data.
		if (InternalData->index_element_size > 0) {
			FreeDataRange(&Context.ObjectIndexBuffer, InternalData->index_buffer_offset, InternalData->index_element_size * InternalData->index_count);
		}

		// Clean up date.
		Memory::Zero(InternalData, sizeof(GeometryData));
		InternalData->id = INVALID_ID;
		InternalData->generation = INVALID_ID;
	}
}

void VulkanBackend::DrawGeometry(GeometryRenderData geometry) {
	// Ignore non-uploaded geometries.
	if (geometry.geometry == nullptr) {
		return;
	}

	if (geometry.geometry->InternalID == INVALID_ID) {
		return;
	}

	GeometryData* BufferData = &Context.Geometries[geometry.geometry->InternalID];
	VulkanCommandBuffer* CmdBuffer = &Context.GraphicsCommandBuffers[Context.ImageIndex];

	// Bind vertex buffer at offset
	vk::DeviceSize offsets[1] = { BufferData->vertext_buffer_offset };
	CmdBuffer->CommandBuffer.bindVertexBuffers(0, Context.ObjectVertexBuffer.Buffer, offsets);

	// Draw index or non-index
	if (BufferData->index_count > 0) {
		// Bind index buffer at offset
		CmdBuffer->CommandBuffer.bindIndexBuffer(Context.ObjectIndexBuffer.Buffer, BufferData->index_buffer_offset, vk::IndexType::eUint32);
		CmdBuffer->CommandBuffer.drawIndexed(BufferData->index_count, 1, 0, 0, 0);
	}
	else {
		CmdBuffer->CommandBuffer.draw(BufferData->vertex_count, 1, 0, 0);
	}
}

bool VulkanBackend::BeginRenderpass(unsigned char renderpass_id) {
	VulkanRenderPass* Renderpass = nullptr;
	VulkanCommandBuffer* CmdBuffer = &Context.GraphicsCommandBuffers[Context.ImageIndex];
	vk::Framebuffer Framebuffer;

	// Choose a renderpass based on ID.
	switch (renderpass_id)
	{
	case eButilin_Renderpass_World:
		Renderpass = &Context.MainRenderPass;
		Framebuffer = Context.WorldFramebuffers[Context.ImageIndex];
		break;
	case eButilin_Renderpass_UI:
		Renderpass = &Context.UIRenderPass;
		Framebuffer = Context.Swapchain.Framebuffers[Context.ImageIndex];
		break;
	default:
		UL_ERROR("Renderer backend begin renderpass called on unrecognized renderpass id: %#02x", renderpass_id);
		return false;
	}

	// Begin renderpass.
	Renderpass->Begin(CmdBuffer, Framebuffer);

	return true;
}

bool VulkanBackend::EndRenderpass(unsigned char renderpass_id) {
	VulkanRenderPass* Renderpass = nullptr;
	VulkanCommandBuffer* CmdBuffer = &Context.GraphicsCommandBuffers[Context.ImageIndex];

	// Choose a renderpass based on ID.
	switch (renderpass_id)
	{
	case eButilin_Renderpass_World:
		Renderpass = &Context.MainRenderPass;
		break;
	case eButilin_Renderpass_UI:
		Renderpass = &Context.UIRenderPass;
		break;
	default:
		UL_ERROR("Renderer backend begin renderpass called on unrecognized renderpass id: %#02x", renderpass_id);
		return false;
	}

	Renderpass->End(CmdBuffer);
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
#endif

// The index of the global descriptor set.
const uint32_t DESC_SET_INDEX_GLOBAL = 0;
// The index of the instance descriptor set.
const uint32_t DESC_SET_INDEX_INSTANCE = 1;

// The index of the UBO binding.
const uint32_t BINDING_INDEX_UBO = 0;

// The index of the image sampler binding.
const uint32_t BINDING_INDEX_SAMPLER = 1;

bool VulkanBackend::CreateShader(Shader* shader, unsigned short renderpass_id, unsigned short stage_count,
	const std::vector<char*>& stage_filenames, std::vector<ShaderStage>& stages) {
	shader->InternalData = Memory::Allocate(sizeof(VulkanShader), MemoryType::eMemory_Type_Renderer);

	// TODO: Dynamic renderpass.
	VulkanRenderPass* Renderpass = renderpass_id == BuiltinRenderpass::eButilin_Renderpass_World
		? &Context.MainRenderPass : &Context.UIRenderPass;

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
			UL_WARN("vulkan_renderer_shader_create: VK_SHADER_STAGE_GEOMETRY_BIT is set but not yet supported.");
			VkStages[i] = vk::ShaderStageFlagBits::eGeometry;
			break;
		case eShader_Stage_Compute:
			UL_WARN("vulkan_renderer_shader_create: SHADER_STAGE_COMPUTE is set but not yet supported.");
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
	OutShader->Renderpass = Renderpass;
	OutShader->Config.max_descriptor_set_count = MaxDescriptorAllocateCount;

	// Shader stages. Parse out the flags.
	Memory::Zero(OutShader->Config.stages, sizeof(VulkanShaderStageConfig) * VULKAN_SHADER_MAX_STAGES);
	OutShader->Config.stage_count = 0;
	for (uint32_t i = 0; i < stage_count; ++i) {
		// Make sure there is room enough to add the stage.
		if (OutShader->Config.stage_count + 1 > VULKAN_SHADER_MAX_STAGES) {
			UL_ERROR("Shaders may have a maximum of %d stages", VULKAN_SHADER_MAX_STAGES);
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
			UL_ERROR("vulkan_shader_create: Unsupported shader stage flagged: %d. Stage ignored.", stages[i]);
			continue;
		}

		// Set the stage and bump the counter.
		OutShader->Config.stages[OutShader->Config.stage_count].stage = StageFlag;
		strncpy(OutShader->Config.stages[OutShader->Config.stage_count].filename, stage_filenames[i], 255);
		OutShader->Config.stage_count++;
	}

	// Zero out arrays and counts.
	Memory::Zero(OutShader->Config.descriptor_sets, sizeof(VulkanDescriptorSetConfig) * 2);

	// Attributes array.
	Memory::Zero(OutShader->Config.attributes, sizeof(vk::VertexInputAttributeDescription) * VULKAN_SHADER_MAX_ATTRIBUTES);


	// For now, shaders will only ever have these 2 types of descriptor pools.
	OutShader->Config.pool_sizes[0] = vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBuffer, 1024 };
	OutShader->Config.pool_sizes[1] = vk::DescriptorPoolSize{ vk::DescriptorType::eCombinedImageSampler, 4096 };

	// Global descriptor set config.
	VulkanDescriptorSetConfig GlobalDescriptorSetConfig;

	// UBO is always available and first.
	GlobalDescriptorSetConfig.bindings[BINDING_INDEX_UBO]
		.setBinding(BINDING_INDEX_UBO)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
		.setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
	GlobalDescriptorSetConfig.binding_count++;

	OutShader->Config.descriptor_sets[DESC_SET_INDEX_GLOBAL] = GlobalDescriptorSetConfig;
	OutShader->Config.descriptor_set_count++;
	if (shader->UseInstances) {
		// If using instances, add a second descriptor set.
		VulkanDescriptorSetConfig InstanceDescriptorSetConfig;

		// Add a UBO to it, as instances should always have one available.
	   // NOTE: Might be a good idea to only add this if it is going to be used...
		InstanceDescriptorSetConfig.bindings[BINDING_INDEX_UBO]
			.setBinding(BINDING_INDEX_UBO)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
		InstanceDescriptorSetConfig.binding_count++;

		OutShader->Config.descriptor_sets[DESC_SET_INDEX_INSTANCE] = InstanceDescriptorSetConfig;
		OutShader->Config.descriptor_set_count++;
	}

	// Invalidate all instance states.
	// TODO: Dynamic
	for (uint32_t i = 0; i < 1024; ++i) {
		OutShader->InstanceStates[i].id = INVALID_ID;
		OutShader->InstanceStates[i].offset	= 0;
	}

	return true;
}

bool VulkanBackend::DestroyShader(Shader* shader) {
	if (shader && shader->InternalData) {
		VulkanShader* Shader = (VulkanShader*)shader->InternalData;
		if (Shader == nullptr) {
			UL_ERROR("vulkan_renderer_shader_destroy requires a valid pointer to a shader.");
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
		Shader->UniformBuffer.UnlockMemory(&Context);
		Shader->MappedUniformBufferBlock = nullptr;
		Shader->UniformBuffer.Destroy(&Context);

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
			UL_ERROR("Unable to create %s shader module for '%s'. Shader will be destroyed.", Shader->Config.stages[i].filename, shader->Name);
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

	// Process uniforms.
	uint32_t UniformCount = (uint32_t)shader->Uniforms.size();
	for (uint32_t i = 0; i < UniformCount; ++i) {
		// For samplers, the descriptor bindings need to be updated. Other types of uniforms don't need anything to be done here.
		if (shader->Uniforms[i].type == ShaderUniformType::eShader_Uniform_Type_Sampler) {
			const uint32_t SetIndex = (shader->Uniforms[i].scope == ShaderScope::eShader_Scope_Global ? DESC_SET_INDEX_GLOBAL : DESC_SET_INDEX_INSTANCE);
			VulkanDescriptorSetConfig* SetConfig = &Shader->Config.descriptor_sets[SetIndex];
			if (SetConfig->binding_count < 2) {
				// There isn't a binding yet, meaning this is the first sampler to be added.
				// Create the binding with a single descriptor for this sampler.
				SetConfig->bindings[BINDING_INDEX_SAMPLER].setBinding(BINDING_INDEX_SAMPLER)
					.setDescriptorCount(1)
					.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
					.setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
				SetConfig->binding_count++;
			}
			else {
				// There is already a binding for samplers, so just add a descriptor to it.
				// Take the current descriptor count as the location and increment the number of descriptors.
				SetConfig->bindings[BINDING_INDEX_SAMPLER].descriptorCount++;
			}
		}
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
		Shader->DescriptorSetLayouts, Shader->Config.stage_count, StageCreateInfos, Viewport, Scissor, false, true, shader->PushConstantsRangeCount, shader->PushConstantsRanges)){
		UL_ERROR("Failed to load graphics pipeline for object shader.");
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
	if (!Shader->UniformBuffer.Create(&Context, TotalBufferSize,
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent | DeviceLocalBits,
		true, true)) {
		UL_ERROR("Vulkan buffer creation failed for object shader.");
		return false;
	}

	// Allocate space for the global UBO, which should occupy the _stride_ space, _not_ the actual size used.
	if (!Shader->UniformBuffer.Allocate(shader->GlobalUboStride, &shader->GlobalUboOffset)) {
		UL_ERROR("Failed to allocate space for the uniform buffer!");
		return false;
	}

	// Map the entire buffer's memory.
	Shader->MappedUniformBufferBlock = Shader->UniformBuffer.LockMemory(&Context, 0, TotalBufferSize/*TotalBufferSize*/, vk::MemoryMapFlags());

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
		UL_ERROR("Allocate descriptor sets failed.");
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
		UL_ERROR("vulkan_shader_bind_instance requires a valid pointer to a shader.");
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
		UL_ERROR("Global image samplers are not yet supported.");

		// VkWriteDescriptorSet sampler_write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
		// descriptor_writes[1] = ...
	}

	Context.Device.GetLogicalDevice().updateDescriptorSets(GlobalSetBindingCount, DescriptorWrites, 0, nullptr);

	// BInd the global descriptor set to be updated.
	CmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, Internal->Pipeline.PipelineLayout, 0, 1, &GlobalDescriptor, 0, nullptr);
	return true;
}

bool VulkanBackend::ApplyInstanceShader(Shader* shader) {
	if (!shader->UseInstances) {
		UL_ERROR("This shader does not use instances.");
		return false;
	}

	VulkanShader* Internal = (VulkanShader*)shader->InternalData;
	uint32_t ImageIndex = Context.ImageIndex;
	vk::CommandBuffer CmdBuffer = Context.GraphicsCommandBuffers[ImageIndex].CommandBuffer;

	// Obtain instance data.
	VulkanShaderInstanceState* ObjectState = &Internal->InstanceStates[shader->BoundInstanceId];
	vk::DescriptorSet ObjectDescriptorSet = ObjectState->descriptor_set_state.descriptorSets[ImageIndex];

	// TODO: if needs update.
	vk::WriteDescriptorSet DescriptorWrites[2];
	Memory::Zero(DescriptorWrites, sizeof(vk::WriteDescriptorSet) * 2);
	uint32_t DescriptorCount = 0;
	uint32_t DescriptorIndex = 0;

	// Descriptor 0 - Uniform buffer
	// Only do this if the descriptor has not yet been updated.
	uint32_t* InstanceUboGeneration = &(ObjectState->descriptor_set_state.descriptor_states[DescriptorIndex].generations[ImageIndex]);
	// TODO: determine if update is required.
	vk::DescriptorBufferInfo BufferInfo;
	vk::WriteDescriptorSet UboDescriptor;
	if (*InstanceUboGeneration == INVALID_ID/* || *GlobalUboGeneration != Material->Generation*/) {
		BufferInfo.setBuffer(Internal->UniformBuffer.Buffer)
			.setOffset(ObjectState->offset)
			.setRange(shader->UboStride);

		UboDescriptor.setDstSet(ObjectDescriptorSet)
			.setDstBinding(DescriptorIndex)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setDescriptorCount(1)
			.setBufferInfo(BufferInfo);

		DescriptorWrites[DescriptorCount] = UboDescriptor;
		DescriptorCount++;

		// Update the frame generation. In this case it is only needed once since this is a buffer.
		*InstanceUboGeneration = 1;  // material->generation; TODO: some generation from... somewhere
	}
	DescriptorIndex++;

	// Samplers will always be in the binding. If the binding count is less than 2, there are no samplers.
	vk::DescriptorImageInfo ImageInfos[VULKAN_SHADER_MAX_GLOBAL_TEXTURES];
	vk::WriteDescriptorSet SamplerDescriptor;
	if (Internal->Config.descriptor_sets[DESC_SET_INDEX_INSTANCE].binding_count > 1) {
		// Iterate samplers.
		uint32_t TotalSamplerCount = Internal->Config.descriptor_sets[DESC_SET_INDEX_INSTANCE].bindings[BINDING_INDEX_SAMPLER].descriptorCount;
		uint32_t UpdateSamplerCount = 0;
		for (uint32_t i = 0; i < TotalSamplerCount; ++i) {
			// TODO: only update in the list if actually needing an update.
			Texture* t = Internal->InstanceStates[shader->BoundInstanceId].instance_textures[i];
			VulkanTexture* InternalData = (VulkanTexture*)t->InternalData;
			ImageInfos[i].setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
				.setImageView(InternalData->Image.ImageView)
				.setSampler(InternalData->sampler);

			// TODO: change up descriptor state to handle this properly.
			// Sync frame generation if not using a default texture.
			// if (t->generation != INVALID_ID) {
			//     *descriptor_generation = t->generation;
			//     *descriptor_id = t->id;
			// }
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

	// Bind the descriptor set to be updated, or in case the shader changed.
	CmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, Internal->Pipeline.PipelineLayout, 1, 1, &ObjectDescriptorSet, 0, nullptr);
	return true;
}

uint32_t VulkanBackend::AcquireInstanceResource(Shader* shader) {
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
		UL_ERROR("vulkan_shader_acquire_instance_resources failed to acquire new id");
		return INVALID_ID;
	}

	VulkanShaderInstanceState* InstanceState = &Internal->InstanceStates[OutInstanceID];
	uint32_t InstanceTextureCount = Internal->Config.descriptor_sets[DESC_SET_INDEX_INSTANCE].bindings[BINDING_INDEX_SAMPLER].descriptorCount;
	// Wipe out the memory for the entire array, even if it isn't all used.
	InstanceState->instance_textures.resize(shader->InstanceTextureCount);
	Texture* DefaultTexture = TextureSystem::GetDefaultTexture();
	// Set all the texture pointers to default until assigned.
	for (uint32_t i = 0; i < InstanceTextureCount; ++i) {
		InstanceState->instance_textures[i] = DefaultTexture;
	}

	// Allocate some space in the UBO - by the stride, not the size.
	uint32_t Size = (uint32_t)shader->UboStride;
	if (!Internal->UniformBuffer.Allocate(Size, &InstanceState->offset)) {
		UL_ERROR("vulkan_material_shader_acquire_resources failed to acquire ubo space");
		return INVALID_ID;
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
			UL_ERROR("Allocate descriptor sets failed.");
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

	if (InstanceState->instance_textures.size() > 0) {
		for (uint32_t i = 0; i < InstanceState->instance_textures.size(); ++i) {
			InstanceState->instance_textures[i] = nullptr;
		}
		InstanceState->instance_textures.clear();
	}

	Internal->UniformBuffer.Free(shader->UboStride, InstanceState->offset);
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
			shader->GlobalTextures[uniform->location] = (Texture*)value;
		}
		else {
			Internal->InstanceStates[shader->BoundInstanceId].instance_textures[uniform->location] = (Texture*)value;
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
	if (!ResourceSystem::Load(config.filename, ResourceType::eResource_type_Binary, &BinaryResource)) {
		UL_ERROR("Unable to read shader module: %s.", config.filename);
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
