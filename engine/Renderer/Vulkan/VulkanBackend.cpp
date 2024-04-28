#include "VulkanBackend.hpp"
#include "VulkanPlatform.hpp"
#include "VulkanDevice.hpp"

#include "Core/Application.hpp"
#include "Core/EngineLogger.hpp"
#include "Containers/TArray.hpp"
#include "Platform/Platform.hpp"
#include "Math/MathTypes.hpp"

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

	Context.MainRenderPass.Create(&Context, 0.0f, 0.0f, (float)Context.FrameBufferWidth, (float)Context.FrameBufferHeight, 1.0f, 0.5f, 0.5f, 1.0f, 1.0f, 0);
	
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

	// Create shaders
	if (!Context.ShaderModule.Create(&Context)) {
		UL_ERROR("Loadding basic_lighting shader failed.");
		return false;
	}

	CreateBuffers();

	// Temp
	const uint32_t VertCount = 3;
	Vertex Verts[VertCount];
	Memory::Zero(Verts, sizeof(Vertex) * VertCount);

	const float f = 1.0f;

	Verts[0].position.x = 0.0f * f;
	Verts[0].position.y = -0.5f * f;
	Verts[0].position.z = 0.0f * f;

	Verts[1].position.x = -0.5f * f;
	Verts[1].position.y = 0.5f * f;
	Verts[1].position.z = 0.0f * f;

	Verts[2].position.x = 0.5f * f;
	Verts[2].position.y = 0.5f * f;
	Verts[2].position.z = 0.0f * f;

	const uint32_t IndexCount = 3;
	uint32_t Indices[IndexCount] = { 0, 2, 1 };

	UploadDataRange(&Context.ObjectVertexBuffer, 0, sizeof(Vertex) * VertCount, Verts);
	UploadDataRange(&Context.ObjectIndexBuffer, 0, sizeof(uint32_t) * IndexCount, Indices);

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
	Context.ShaderModule.Destroy(&Context);

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

	UL_DEBUG("Destroying frame buffers.");
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
	if (!Context.InFlightFences[Context.CurrentFrame].Wait(&Context, UINT64_MAX)) {
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

	// Begin the render pass
	Context.MainRenderPass.Begin(CommandBuffer, Context.Swapchain.Framebuffers[Context.ImageIndex].FrameBuffer);

	return true;
}

void VulkanBackend::UpdateGlobalState(Matrix4 projection, Matrix4 view, Vec3 view_position, Vec4 ambient_color, int mode) {
	VulkanCommandBuffer* CmdBuffer = &Context.GraphicsCommandBuffers[Context.ImageIndex];

	Context.ShaderModule.Use(&Context);

	Context.ShaderModule.GlobalUBO.projection = projection;
	Context.ShaderModule.GlobalUBO.view = view;

	// TODO: other ubo props

	Context.ShaderModule.UpdateGlobalState(&Context);
}

void VulkanBackend::UpdateObject(Matrix4 model_trans) {
	VulkanCommandBuffer* CmdBuffer = &Context.GraphicsCommandBuffers[Context.ImageIndex];

	Context.ShaderModule.UpdateObject(&Context, model_trans);

	// Temp test
	Context.ShaderModule.Use(&Context);
	// Bind vertex buffer at offset
	vk::DeviceSize offset[1] = { 0 };
	CmdBuffer->CommandBuffer.bindVertexBuffers(0, 1, &Context.ObjectVertexBuffer.Buffer, (vk::DeviceSize*)offset);

	// Bind index buffer at offset
	CmdBuffer->CommandBuffer.bindIndexBuffer(Context.ObjectIndexBuffer.Buffer, 0, vk::IndexType::eUint32);

	// Issue the draw
	CmdBuffer->CommandBuffer.drawIndexed(3, 1, 0, 0, 0);

}

bool VulkanBackend::EndFrame(double delta_time) {

	VulkanCommandBuffer* CommandBuffer = &Context.GraphicsCommandBuffers[Context.ImageIndex];

	// End renderpass
	Context.MainRenderPass.End(CommandBuffer);
	
	CommandBuffer->EndCommand();

	// Make sure the previous frame is not using this image
	if (Context.ImagesInFilght[Context.ImageIndex] != VK_NULL_HANDLE) {
		Context.ImagesInFilght[Context.ImageIndex]->Wait(&Context, UINT64_MAX);
	}

	// Make sure image fence as in use by this frame
	Context.ImagesInFilght[Context.ImageIndex] = &Context.InFlightFences[Context.CurrentFrame];

	// Reset the fence for use on the next frame
	Context.InFlightFences[Context.CurrentFrame].Reset(&Context);

	// Submit the queue and wait for the operation to complete
	// Regin queue submission
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

	if (Context.Device.GetGraphicsQueue().submit(1, &SubmitInfo, Context.InFlightFences[Context.CurrentFrame].fence) != vk::Result::eSuccess) {
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
		Context.Swapchain.Framebuffers[i].Destroy(&Context);
	}

	Context.MainRenderPass.SetX(0);
	Context.MainRenderPass.SetY(0);
	Context.MainRenderPass.SetW((float)Context.FrameBufferWidth);
	Context.MainRenderPass.SetH((float)Context.FrameBufferHeight);

	RegenerateFrameBuffers();
	CreateCommandBuffer();

	// Clear the recreating flag.
	Context.RecreatingSwapchain = false;

	return true;
}

bool VulkanBackend::CreateBuffers() {
	vk::MemoryPropertyFlagBits MemoryPropertyFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;

	const size_t VertexBufferSize = sizeof(Vertex) * 1024 * 1024;
	if (!Context.ObjectVertexBuffer.Create(&Context, VertexBufferSize, 
		vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc,
		MemoryPropertyFlags, true)) {
		UL_ERROR("Error creating vertex buffer.");
		return false;
	}

	Context.GeometryVertexOffset = 0;

	const size_t IndexBufferSize = sizeof(uint32_t) * 1024 * 1024;
	if (!Context.ObjectIndexBuffer.Create(&Context, IndexBufferSize,
		vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc,
		MemoryPropertyFlags, true)) {
		UL_ERROR("Error creating index buffer.");
		return false;
	}

	Context.GeometryIndexOffset = 0;

	return true;
}

void VulkanBackend::UploadDataRange(VulkanBuffer* buffer, size_t offset, size_t size, void* data) {
	// Create a host-visible staging buffer to upload to. Mark it as the source of the transfer.
	vk::MemoryPropertyFlags Flags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
	VulkanBuffer Staging;
	Staging.Create(&Context, size, vk::BufferUsageFlagBits::eTransferSrc, Flags, true);

	// Load the data into the staging buffer.
	Staging.LoadData(&Context, 0, size, vk::MemoryMapFlags(), data);

	// Perform the copy from staging to the device local buffer.
	Staging.CopyTo(&Context, Context.Device.GetGraphicsCommandPool(), nullptr, Context.Device.GetGraphicsQueue(), 
		Staging.Buffer, 0 , buffer->Buffer, offset, size);

	// Clean up the staging buffer.
	Staging.Destroy(&Context);
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