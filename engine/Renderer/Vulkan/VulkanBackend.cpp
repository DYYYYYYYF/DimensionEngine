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
			if (strcmp(RequiredValidationLayerName[i], AvailableLayers[j].layerName.data()) == 0) {
				IsFound = true;
				UL_INFO("Found.");
				break;
			}
		}

		if (!IsFound) {
			UL_WARN("Required validation layer is missing: '%s'!", RequiredValidationLayerName[i]);

			// TODO: Remove spect-element.
			RequiredValidationLayerName.clear();
		}
	}

#endif
	InstanceInfo.setEnabledLayerCount((uint32_t)RequiredValidationLayerName.size())
		.setPEnabledLayerNames(RequiredValidationLayerName);

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

	// Create shaders
	if (!Context.MaterialShader.Create(&Context)) {
		UL_ERROR("Loadding basic_lighting shader failed.");
		return false;
	}
	if (!Context.UIShader.Create(&Context)) {
		UL_ERROR("Loadding ui shader failed.");
		return false;
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

	UL_DEBUG("Destroying shader modules.");
	Context.UIShader.Destroy(&Context);
	Context.MaterialShader.Destroy(&Context);

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

void VulkanBackend::UpdateGlobalWorldState(Matrix4 projection, Matrix4 view, Vec3 view_position, Vec4 ambient_color, int mode) {
	VulkanCommandBuffer* CmdBuffer = &Context.GraphicsCommandBuffers[Context.ImageIndex];

	Context.MaterialShader.Use(&Context);

	Context.MaterialShader.GlobalUBO.projection = projection;
	Context.MaterialShader.GlobalUBO.view = view;

	// TODO: other ubo props

	Context.MaterialShader.UpdateGlobalState(&Context, Context.FrameDeltaTime);
}

void VulkanBackend::UpdateGlobalUIState(Matrix4 projection, Matrix4 view, int mode) {
	VulkanCommandBuffer* CmdBuffer = &Context.GraphicsCommandBuffers[Context.ImageIndex];

	Context.UIShader.Use(&Context);

	Context.UIShader.GlobalUBO.projection = projection;
	Context.UIShader.GlobalUBO.view = view;

	// TODO: other ubo props

	Context.UIShader.UpdateGlobalState(&Context, Context.FrameDeltaTime);
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
		MemoryPropertyFlags, true)) {
		UL_ERROR("Error creating vertex buffer.");
		return false;
	}

	// Geometry index buffer
	const size_t IndexBufferSize = sizeof(uint32_t) * 1024 * 1024;
	if (!Context.ObjectIndexBuffer.Create(&Context, IndexBufferSize,
		vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc,
		MemoryPropertyFlags, true)) {
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
	Staging.Create(&Context, size, vk::BufferUsageFlagBits::eTransferSrc, Flags, true);

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
	Staging.Create(&Context, ImageSize, Usage, MemoryPropFlags, true);

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


bool VulkanBackend::CreateMaterial(Material* material) {
	if (material) {
		switch (material->Type)
		{
		case eMaterial_Type_World:
			if (!Context.MaterialShader.AcquireResources(&Context, material)) {
				UL_INFO("Renderer create material failed to acquire world shader resources.");
				return false;
			}
			break;
		case eMaterial_Type_UI:
			if (!Context.UIShader.AcquireResources(&Context, material)) {
				UL_INFO("Renderer create material failed to acquire ui shader resources.");
				return false;
			}
			break;
		default:
			UL_INFO("Renderer create material failed: Unknown material type.");
			return false;
		}

		UL_INFO("Renderer: Material created.");
		return true;
	}

	UL_ERROR("Vulkan renderer create material - material is nullptr.");
	return false;
}

void VulkanBackend::DestroyMaterial(Material* material) {
	if (material) {
		if (material->InternalId != INVALID_ID) {
			switch (material->Type)
			{
			case eMaterial_Type_World:
				Context.MaterialShader.ReleaseResources(&Context, material);
				break;
			case eMaterial_Type_UI:
				Context.UIShader.ReleaseResources(&Context, material);
				break;
			default:
				UL_ERROR("Destroy material failed: Unknown material type.");
				break;
			}
		}
		else {
			UL_WARN("Vulkan renderer destroy material called with InternalId = INVALID_ID. Nothing was done.");
		}
	}
	else {
		UL_WARN("Vulkan renderer destroy material called with nullptr = INVALID_ID. Nothing was done.");
	}
}

bool VulkanBackend::CreateGeometry(Geometry* geometry, uint32_t vertex_size, uint32_t vertex_count, 
	const void* vertices, uint32_t index_size, uint32_t index_count, const void* indices) {
	if (vertex_count == 0 || index_count == 0) {
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

	Material* mat = nullptr;
	if (geometry.geometry->Material) {
		mat = geometry.geometry->Material;
	}
	else {
		mat = MaterialSystem::GetDefaultMaterial();
	}
	

	switch (mat->Type)
	{
	case eMaterial_Type_World:
		Context.MaterialShader.SetModelMat(&Context, geometry.model);
		Context.MaterialShader.ApplyMaterial(&Context, mat);
		break;
	case eMaterial_Type_UI:
		Context.UIShader.SetModelMat(&Context, geometry.model);
		Context.UIShader.ApplyMaterial(&Context, mat);
		break;
	default:
		UL_ERROR("Draw geometry error: Unknown material type.");
		return;
	}

	// Bind vertex buffer at offset
	vk::DeviceSize offsets[1] = { BufferData->vertext_buffer_offset };
	CmdBuffer->CommandBuffer.bindVertexBuffers(0, Context.ObjectVertexBuffer.Buffer, offsets);

	// Draw index or non-index
	if (BufferData->index_element_size > 0) {
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

	// Use the appropriate shader.
	switch (renderpass_id)
	{
	case eButilin_Renderpass_World:
		Context.MaterialShader.Use(&Context);
		break;
	case eButilin_Renderpass_UI:
		Context.UIShader.Use(&Context);
		break;
	}

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
