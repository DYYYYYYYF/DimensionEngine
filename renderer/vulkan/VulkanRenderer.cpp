#include "VulkanRenderer.hpp"
#include "PipelineBuilder.hpp"
#include "VkStructures.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

using namespace renderer;

VulkanRenderer::VulkanRenderer() {
    INFO("Use Vulkan Renderer.");
    ResetProp();
}

VulkanRenderer::~VulkanRenderer() {
    ResetProp();
}

void VulkanRenderer::ResetProp() {
    _Window = nullptr;
    _VkInstance = nullptr;
    _VkPhyDevice = nullptr;
    _SurfaceKHR = nullptr;
    _VkDevice = nullptr;
    _SwapchainKHR = nullptr;
    _VkRenderPass = nullptr;
    _PipelineLayout = nullptr;
    _Pipeline = nullptr;
    _VertShader = nullptr;
    _FragShader = nullptr;
    _DepthImageView = nullptr;
    _FrameNumber = 0;
}

bool VulkanRenderer::Init() { 
    CreateInstance();
    PickupPhyDevice();
    CreateSurface();
    CreateDevice();
    InitQueue();
    CreateSwapchain();
    GetVkImages();
    GetVkImageViews();
    CreateRenderPass();
    CreateCmdPool();
    AllocateFrameCmdBuffer();
    CreateDepthImage();
    CreateFrameBuffers();
    InitSyncStructures();

    return true; 
}
void VulkanRenderer::Release(){
    CHECK(_VkDevice);
    INFO("Release Renderer");
    
    _VkDevice.destroyImageView(_DepthImageView);

    _VkDevice.destroyPipelineLayout(_PipelineLayout);
    _VkDevice.destroyPipeline(_Pipeline);

    for (int i = 0; i < FRAME_OVERLAP; ++i) {
        _VkDevice.destroyFence(_Frames[i].renderFence);
        _VkDevice.destroySemaphore(_Frames[i].renderSemaphore);
        _VkDevice.destroySemaphore(_Frames[i].presentSemaphore);
        _VkDevice.destroyCommandPool(_Frames[i].commandPool);
    }

    _VkDevice.destroyRenderPass(_VkRenderPass);
    for (auto& frameBuffer : _FrameBuffers) { _VkDevice.destroyFramebuffer(frameBuffer); }
    for (auto& view : _ImageViews) { _VkDevice.destroyImageView(view); }
    for (auto& image : _Images) { _VkDevice.destroyImage(image); }
    _VkDevice.destroySwapchainKHR(_SwapchainKHR);
    INFO("Release Swapchain");

    _VkDevice.destroy();
    INFO("Release Device");

    _VkInstance.destroySurfaceKHR(_SurfaceKHR);
    INFO("Destroy Instance");

    _VkInstance.destroy();
    INFO("Release Renderer");

}


void VulkanRenderer::InitWindow(){
    _Window =  udon::WsiWindow::GetInstance()->GetWindow();
    if (!_Window) {
        DEBUG("Error Window.");
        exit(-1);
    }
}

void VulkanRenderer::CreateInstance() {
    InitWindow();

    // Get SDL instance extensions
    unsigned int extensionsCount;
    const char** extensionNames;
    extensionNames = glfwGetRequiredInstanceExtensions(&extensionsCount);

#ifdef __APPLE__
    // MacOS requirment
    extensionNames.push_back("VK_KHR_get_physical_device_properties2");
    ++extensionsCount;
#endif


    std::vector<VkLayerProperties> enumLayers;
    uint32_t enumLayerCount;
    vkEnumerateInstanceLayerProperties(&enumLayerCount, enumLayers.data());
    enumLayers.resize(enumLayerCount);
    vkEnumerateInstanceLayerProperties(&enumLayerCount, enumLayers.data());

    //validation layers
    std::vector<const char*> layers;
    for (auto& enumLayer : enumLayers) {
        if (strcmp(enumLayer.layerName, "VK_LAYER_KHRONOS_validation") == 0) {
            layers.push_back("VK_LAYER_KHRONOS_validation");
        }
    }

    vk::InstanceCreateInfo info;
    info.setPpEnabledExtensionNames(extensionNames)
        .setPEnabledLayerNames(layers)
        .setEnabledExtensionCount(extensionsCount)
        .setEnabledLayerCount((uint32_t)layers.size());

#ifdef _DEBUG
    std::cout << "Added Extensions:\n";
    for (auto& extension : extensionNames) std::cout << extension << std::endl;
    std::cout << "Added Layers:\n";
    for (auto& layer : layers) std::cout << layer << std::endl;
#endif

    _VkInstance = vk::createInstance(info);
}

void VulkanRenderer::PickupPhyDevice() {
    std::vector<vk::PhysicalDevice> physicalDevices = _VkInstance.enumeratePhysicalDevices();
 #ifdef _DEBUG
    std::cout << "\nUsing GPU:  " << physicalDevices[0].getProperties().deviceName << std::endl;    //输出显卡名称
#endif
    _VkPhyDevice = physicalDevices[0];
}

void VulkanRenderer::CreateSurface(){
    CHECK(_Window); 
    CHECK(_VkInstance);

    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(_VkInstance, _Window, nullptr, &surface) != VK_SUCCESS){
        INFO("Create surface failed.");
        return;
    }

    _SurfaceKHR = vk::SurfaceKHR(surface);
    CHECK(_SurfaceKHR);
}

void VulkanRenderer::CreateDevice(){
    if (!QueryQueueFamilyProp()){
        INFO("Query Queue Family properties failed.");
        return;
    }

    std::vector<vk::DeviceQueueCreateInfo> dqInfo;
    // Only one instance when graphicsIndex sames as presentIndex
    if (_QueueFamilyProp.graphicsIndex.value() == _QueueFamilyProp.presentIndex.value()){
        vk::DeviceQueueCreateInfo info;
        float priority = 1.0;
        info.setQueuePriorities(priority);
        info.setQueueCount(1);
        info.setQueueFamilyIndex(_QueueFamilyProp.graphicsIndex.value());    
        dqInfo.push_back(info);
    } else {
        vk::DeviceQueueCreateInfo info1;
        float priority = 1.0;
        info1.setQueuePriorities(priority); 
        info1.setQueueCount(1);
        info1.setQueueFamilyIndex(_QueueFamilyProp.graphicsIndex.value());

        vk::DeviceQueueCreateInfo info2;
        info2.setQueuePriorities(priority);
        info2.setQueueCount(1);
        info2.setQueueFamilyIndex(_QueueFamilyProp.presentIndex.value());

        dqInfo.push_back(info1);       
        dqInfo.push_back(info2);        
    }

    vk::DeviceCreateInfo dInfo;
    dInfo.setQueueCreateInfos(dqInfo);

#ifdef __APPLE__
    std::array<const char*,2> extensions{"VK_KHR_portability_subset",
                                          VK_KHR_SWAPCHAIN_EXTENSION_NAME};
#elif _WIN32
    std::array<const char*, 1> extensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME};
#endif
    dInfo.setPEnabledExtensionNames(extensions);

    _VkDevice = _VkPhyDevice.createDevice(dInfo);
    CHECK(_VkDevice);
}

void VulkanRenderer::CreateSwapchain() {
    _SupportInfo.QuerySupportInfo(_VkPhyDevice, _SurfaceKHR);

    vk::SwapchainCreateInfoKHR scInfo;
    scInfo.setImageColorSpace(_SupportInfo.format.colorSpace);
    scInfo.setImageFormat(_SupportInfo.format.format);
    scInfo.setImageExtent(_SupportInfo.extent);
    scInfo.setMinImageCount(_SupportInfo.imageCount);
    scInfo.setPresentMode(_SupportInfo.presnetMode);
    scInfo.setPreTransform(_SupportInfo.capabilities.currentTransform);

    if (_QueueFamilyProp.graphicsIndex.value() == _QueueFamilyProp.presentIndex.value()) {
        scInfo.setQueueFamilyIndices(_QueueFamilyProp.graphicsIndex.value());
        scInfo.setImageSharingMode(vk::SharingMode::eExclusive);
    }
    else {
        std::array<uint32_t, 2> index{ _QueueFamilyProp.graphicsIndex.value(),
                                     _QueueFamilyProp.presentIndex.value() };
        scInfo.setQueueFamilyIndices(index);
        scInfo.setImageSharingMode(vk::SharingMode::eConcurrent);
    }

    scInfo.setClipped(true);
    scInfo.setSurface(_SurfaceKHR);
    scInfo.setImageArrayLayers(1);
    scInfo.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
    scInfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);

    _SwapchainKHR = _VkDevice.createSwapchainKHR(scInfo);
    CHECK(_SwapchainKHR);
}

void VulkanRenderer::CreateRenderPass() {
    CHECK(_VkDevice);
    CHECK(_VkPhyDevice);
    
    //颜色附件
    vk::AttachmentDescription attchmentDesc;
    attchmentDesc.setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setFormat(_SupportInfo.format.format)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentReference colRefer;
    colRefer.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
    colRefer.setAttachment(0);

    //深度附件
      vk::AttachmentDescription depthAttchDesc;
      vk::Format depthFormat = FindSupportedFormat({ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eX8D24UnormPack32 },
          vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
      depthAttchDesc.setFormat(depthFormat)
                    .setSamples(vk::SampleCountFlagBits::e1)
                    .setLoadOp(vk::AttachmentLoadOp::eClear)
                    .setStoreOp(vk::AttachmentStoreOp::eDontCare)
                    .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                    .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                    .setInitialLayout(vk::ImageLayout::eUndefined)
                    .setFinalLayout(vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal);
      vk::AttachmentReference depthRef;
      depthRef.setAttachment(1)
              .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
 
    vk::SubpassDescription subpassDesc;
    subpassDesc.setColorAttachmentCount(1);
    subpassDesc.setPColorAttachments(&colRefer);
    subpassDesc.setPDepthStencilAttachment(&depthRef);
    subpassDesc.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);

    std::array<vk::AttachmentDescription, 2> attachments = {attchmentDesc, depthAttchDesc};
    
    vk::SubpassDependency dependency;
    dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL);
    dependency.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    dependency.setSrcAccessMask(vk::AccessFlagBits::eNone);
    dependency.setDstSubpass(0);
    dependency.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    dependency.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
    dependency.setDependencyFlags(vk::DependencyFlagBits::eByRegion);

    vk::SubpassDependency depthDependency;
    depthDependency.setSrcStageMask(
        vk::PipelineStageFlagBits::eEarlyFragmentTests | 
        vk::PipelineStageFlagBits::eLateFragmentTests);
    depthDependency.setSrcAccessMask(vk::AccessFlagBits::eNone);
    depthDependency.setDstSubpass(0);
    depthDependency.setDstStageMask(vk::PipelineStageFlagBits::eEarlyFragmentTests |
        vk::PipelineStageFlagBits::eLateFragmentTests);
    depthDependency.setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite);
    depthDependency.setDependencyFlags(vk::DependencyFlagBits::eByRegion);

    std::vector<vk::SubpassDependency> dependecies = { dependency, depthDependency };

    vk::RenderPassCreateInfo info;
    info.setSubpassCount(1)
        .setSubpasses(subpassDesc)
        .setAttachmentCount((uint32_t)attachments.size())
        .setAttachments(attachments)
        .setDependencies(dependecies)
        .setDependencyCount((uint32_t)dependecies.size());

    _VkRenderPass = _VkDevice.createRenderPass(info);
    CHECK(_VkRenderPass);
}

void VulkanRenderer::CreateCmdPool() {
    CHECK(_VkDevice);

    for (int i = 0; i < FRAME_OVERLAP; ++i) {
        vk::CommandPool& cmdPool = _Frames[i].commandPool;

        vk::CommandPoolCreateInfo info;
        info.setQueueFamilyIndex(_QueueFamilyProp.graphicsIndex.value())
            .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        cmdPool = _VkDevice.createCommandPool(info);

        CHECK(cmdPool);
    }
}

void VulkanRenderer::AllocateFrameCmdBuffer() {
    for (int i = 0; i < FRAME_OVERLAP; ++i) {
        CHECK(_Frames[i].commandPool);
        vk::CommandBuffer& cmdBuffer = _Frames[i].mainCommandBuffer;

        vk::CommandBufferAllocateInfo allocte;
        allocte.setCommandPool(_Frames[i].commandPool)
            .setCommandBufferCount(1)
            .setLevel(vk::CommandBufferLevel::ePrimary);

        cmdBuffer = _VkDevice.allocateCommandBuffers(allocte)[0];
        CHECK(cmdBuffer);
    }
}

vk::CommandBuffer VulkanRenderer::AllocateCmdBuffer() {
    CHECK(_VkDevice);
    CHECK(GetCurrentFrame().commandPool);
    if (!_VkDevice) {
        return nullptr;
    }

    vk::CommandBufferAllocateInfo allocte;
    allocte.setCommandPool(GetCurrentFrame().commandPool)
        .setCommandBufferCount(1)
        .setLevel(vk::CommandBufferLevel::ePrimary);

    vk::CommandBuffer CmdBuffer;
    CmdBuffer = _VkDevice.allocateCommandBuffers(allocte)[0];

    vk::CommandBufferBeginInfo info;
    info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    CmdBuffer.begin(info);

    return CmdBuffer;
}

void VulkanRenderer::EndCmdBuffer(vk::CommandBuffer cmdBuf) {
    CHECK(cmdBuf)
    vk::SubmitInfo submitInfo;
    submitInfo.setCommandBuffers(cmdBuf);

    _Queue.GraphicsQueue.submit(submitInfo);
    _VkDevice.waitIdle();
    _VkDevice.freeCommandBuffers(GetCurrentFrame().commandPool, cmdBuf);
}

void VulkanRenderer::CreateFrameBuffers() {
    CHECK(_SwapchainKHR);
    const int nSwapchainCount = (int)_Images.size();
    _FrameBuffers = std::vector<vk::Framebuffer>(nSwapchainCount);

    for (int i = 0; i < nSwapchainCount; ++i) {
        std::array<vk::ImageView, 2> attchments = { _ImageViews[i], _DepthImageView };
        vk::FramebufferCreateInfo info;
        info.setRenderPass(_VkRenderPass)
            .setAttachmentCount((uint32_t)attchments.size())
            .setPAttachments(attchments.data())
            .setWidth(_SupportInfo.GetWindowWidth())
            .setHeight(_SupportInfo.GetWindowHeight())
            .setLayers(1);
        _FrameBuffers[i] = _VkDevice.createFramebuffer(info);
        CHECK(_FrameBuffers[i]);
    }
}

void VulkanRenderer::InitSyncStructures() {

    for (int i = 0; i < FRAME_OVERLAP; ++i) {
        vk::Fence& renderFence = _Frames[i].renderFence;
        vk::Semaphore& renderSemap = _Frames[i].renderSemaphore;
        vk::Semaphore& presentSemap = _Frames[i].presentSemaphore;
        
        vk::FenceCreateInfo FenceInfo;
        FenceInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
        renderFence = _VkDevice.createFence(FenceInfo);
        CHECK(renderFence);

        vk::SemaphoreCreateInfo SemapInfo;
        renderSemap = _VkDevice.createSemaphore(SemapInfo);
        CHECK(renderSemap);

        presentSemap = _VkDevice.createSemaphore(SemapInfo);
        CHECK(presentSemap);
    }
}

void VulkanRenderer::CreatePipeline(Material& mat) {
    PipelineBuilder pipelineBuilder;

    _VertShader = CreateShaderModule("../shader/triangle_vert.spv");
    _FragShader = CreateShaderModule("../shader/triangle_frag.spv");
    CHECK(_VertShader);
    CHECK(_FragShader);

    INFO("Push constants");
    vk::PushConstantRange pushConstant;
    pushConstant.setOffset(0);
    pushConstant.setSize(sizeof(MeshPushConstants));
    pushConstant.setStageFlags(vk::ShaderStageFlagBits::eVertex);
    CHECK(pushConstant);

    INFO("Pipeline Layout");
    vk::PipelineLayoutCreateInfo info = InitPipelineLayoutCreateInfo();
    info.setPushConstantRanges(pushConstant);
    info.setPushConstantRangeCount(1);
    _PipelineLayout = _VkDevice.createPipelineLayout(info);
    CHECK(_PipelineLayout);

    INFO("Pipeline Shader Stages");
    pipelineBuilder._ShaderStages.clear();
    pipelineBuilder._ShaderStages.push_back(InitShaderStageCreateInfo(vk::ShaderStageFlagBits::eVertex, _VertShader));
    pipelineBuilder._ShaderStages.push_back(InitShaderStageCreateInfo(vk::ShaderStageFlagBits::eFragment, _FragShader));

    INFO("Pipeline Assembly");
    pipelineBuilder._InputAssembly = InitAssemblyStateCreateInfo(vk::PrimitiveTopology::eTriangleList);

    INFO("Pipeline Viewport");
    pipelineBuilder._Viewport.x = 0.0f;
    pipelineBuilder._Viewport.y = 0.0f;
    pipelineBuilder._Viewport.width = (float)_SupportInfo.extent.width;
    pipelineBuilder._Viewport.height = (float)_SupportInfo.extent.width;
    pipelineBuilder._Viewport.minDepth = 0.0f;
    pipelineBuilder._Viewport.maxDepth = 1.0f;

    INFO("Pipeline Scissor");
    pipelineBuilder._Scissor.offset = vk::Offset2D{ 0, 0 };
    pipelineBuilder._Scissor.extent = _SupportInfo.extent;

    INFO("Pipeline Rasterizer");
    pipelineBuilder._Rasterizer = InitRasterizationStateCreateInfo(vk::PolygonMode::eFill);
    pipelineBuilder._Mutisampling = InitMultisampleStateCreateInfo();
    pipelineBuilder._ColorBlendAttachment = InitColorBlendAttachmentState();
    pipelineBuilder._PipelineLayout = _PipelineLayout;
    pipelineBuilder._DepthStencilState = InitDepthStencilStateCreateInfo();

    INFO("Pipeline Bingding and Attribute Descroptions");
    std::vector<vk::VertexInputBindingDescription> bindingDescription = Vertex::GetBindingDescription();
    std::array<vk::VertexInputAttributeDescription, 3> attributeDescription = Vertex::GetAttributeDescription();
    
    //connect the pipeline builder vertex input info to the one we get from Vertex
    pipelineBuilder._VertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data();
    pipelineBuilder._VertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)attributeDescription.size();
    pipelineBuilder._VertexInputInfo.pVertexBindingDescriptions = bindingDescription.data();
    pipelineBuilder._VertexInputInfo.vertexBindingDescriptionCount = (uint32_t)bindingDescription.size();

    pipelineBuilder._PipelineLayout = _PipelineLayout;

    _Pipeline = pipelineBuilder.BuildPipeline(_VkDevice, _VkRenderPass);

    mat.pipeline = _Pipeline;
    mat.pipelineLayout = _PipelineLayout;
}

void VulkanRenderer::DrawObjects(vk::CommandBuffer& cmd, RenderObject* first, int count) {
    //make a model view matrix for rendering the object

    Mesh* lastMesh = nullptr;
    Material* lastMaterial = nullptr;
    for (int i = 0; i < count; i++)
    {
        RenderObject& object = first[i];

        //only bind the pipeline if it doesn't match with the already bound one
        if (object.material != lastMaterial) {
            cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, object.material->pipeline);
            lastMaterial = object.material;
        }

        //camera projection
        _PushConstants.proj= glm::perspective(glm::radians(45.f),
            _SupportInfo.GetWindowHeight() / (float)_SupportInfo.GetWindowWidth(), 0.1f, 200.0f);
        _PushConstants.proj[1][1] *= -1;
        _PushConstants.model = object.transformMatrix;

        //upload the mesh to the GPU via push constants
        cmd.pushConstants(object.material->pipelineLayout,
            vk::ShaderStageFlagBits::eVertex, 0, sizeof(MeshPushConstants), &_PushConstants);

        //only bind the mesh if it's a different one from last bind
        if (object.mesh != lastMesh) {
            //bind the mesh vertex buffer with offset 0
            VkDeviceSize offset = 0;
            cmd.bindVertexBuffers(0, object.mesh->vertexBuffer.buffer, offset);
            lastMesh = object.mesh;
        }

        //we can now draw
        cmd.draw((uint32_t)object.mesh->vertices.size(), 1, 0, 0);
    }
}

void VulkanRenderer::DrawPerFrame(RenderObject* first, int count) {
    _VkDevice.resetFences(GetCurrentFrame().renderFence);

    auto res = _VkDevice.acquireNextImageKHR(_SwapchainKHR, std::numeric_limits<uint64_t>::max(), 
        GetCurrentFrame().presentSemaphore, nullptr);
    uint32_t nSwapchainImageIndex = res.value;

    vk::CommandBuffer& cmdBuffer = GetCurrentFrame().mainCommandBuffer;
    CHECK(GetCurrentFrame().commandBuffer);
    cmdBuffer.reset();

    vk::CommandBufferBeginInfo info;
    info.setPInheritanceInfo(nullptr)
        .setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse);

    cmdBuffer.begin(info);

    std::array<vk::ClearValue, 2> ClearValues;
    vk::ClearColorValue color = std::array<float, 4>{1.f, 1.f, 1.f, 1.0f};
    ClearValues[0].setColor(color);
    ClearValues[1].setDepthStencil({ 1.0, 0 });

    vk::RenderPassBeginInfo rpInfo;
    rpInfo.setRenderPass(_VkRenderPass)
        .setRenderArea(vk::Rect2D({ 0, 0 }, _SupportInfo.extent))
        .setFramebuffer(_FrameBuffers[nSwapchainImageIndex])
        .setClearValueCount((uint32_t)ClearValues.size())
        .setClearValues(ClearValues);

    cmdBuffer.beginRenderPass(rpInfo, vk::SubpassContents::eInline);

    DrawObjects(cmdBuffer, first, count);

    cmdBuffer.endRenderPass();
    cmdBuffer.end();

    vk::PipelineStageFlags WaitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submit;
    submit.setWaitSemaphoreCount(1)
        .setWaitSemaphores(GetCurrentFrame().presentSemaphore)
        .setSignalSemaphoreCount(1)
        .setSignalSemaphores(GetCurrentFrame().renderSemaphore)
        .setCommandBufferCount(1)
        .setCommandBuffers(cmdBuffer)
        .setWaitDstStageMask(WaitStage);

    if (_Queue.GraphicsQueue.submit(1, &submit, GetCurrentFrame().renderFence) != vk::Result::eSuccess) {
        return;
    }
    vk::PresentInfoKHR PresentInfo;
    PresentInfo.setSwapchainCount(1)
        .setSwapchains(_SwapchainKHR)
        .setWaitSemaphoreCount(1)
        .setWaitSemaphores(GetCurrentFrame().renderSemaphore)
        .setImageIndices(nSwapchainImageIndex);

    if (_Queue.GraphicsQueue.presentKHR(PresentInfo) != vk::Result::eSuccess) {
        return;
    }

    if (_VkDevice.waitForFences(GetCurrentFrame().renderFence, true, 
        std::numeric_limits<uint64_t>::max()) != vk::Result::eSuccess) {
        return;
    }
    
    _VkDevice.waitIdle();
    _FrameNumber++;
}

void VulkanRenderer::CreateDepthImage(){
    // Depth Image
    vk::Format depthFormat = FindSupportedFormat({ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eX8D24UnormPack32 },
        vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
    uint32_t width = _SupportInfo.GetWindowWidth();
    uint32_t height = _SupportInfo.GetWindowHeight();
    vk::Extent3D depthImageExtent = { width, height, 1 };

    _DepthImage.image = CreateImage(vk::Format::eD32Sfloat,
        vk::ImageUsageFlagBits::eDepthStencilAttachment, depthImageExtent);
    CHECK(_DepthImage.image);
    MemRequiredInfo memInfo = QueryImgReqInfo(_DepthImage.image, vk::MemoryPropertyFlagBits::eDeviceLocal);
    _DepthImage.memory = AllocateMemory(memInfo, vk::MemoryPropertyFlagBits::eDeviceLocal);
    CHECK(_DepthImage.memory);
    _VkDevice.bindImageMemory(_DepthImage.image, _DepthImage.memory, 0);

    _DepthImageView = CreateImageView(depthFormat, _DepthImage.image, vk::ImageAspectFlagBits::eDepth);
    TransitionImageLayout(_DepthImage.image, depthFormat, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal);
}

/*
 *  Utils functions
 * */
bool VulkanRenderer::QueryQueueFamilyProp(){
    CHECK(_VkPhyDevice)
    CHECK(_SurfaceKHR);
     
    auto families = _VkPhyDevice.getQueueFamilyProperties();
    uint32_t index = 0;
    for(auto &family:families){
        //Pickup Graph command
        if(family.queueFlags | vk::QueueFlagBits::eGraphics){
            _QueueFamilyProp.graphicsIndex = index;
        }
        //Pickup Surface command
        if(_VkPhyDevice.getSurfaceSupportKHR(index, _SurfaceKHR)){
            _QueueFamilyProp.presentIndex = index;
        }
        //if no command, break
        if(_QueueFamilyProp.graphicsIndex && _QueueFamilyProp.presentIndex) {
            break;
        }
        index++;
    }

#ifdef _DEBUG 
    std::cout << "Graphics Index:" << _QueueFamilyProp.graphicsIndex.value() << 
        "\nPresent Index: " << _QueueFamilyProp.presentIndex.value() << std::endl;
#endif
    return _QueueFamilyProp.graphicsIndex && _QueueFamilyProp.presentIndex;
}

bool VulkanRenderer::InitQueue() {
    CHECK(_VkDevice);
    return _Queue.InitQueue(_VkDevice, _QueueFamilyProp);
}

void VulkanRenderer::GetVkImages() {
    _Images = _VkDevice.getSwapchainImagesKHR(_SwapchainKHR);
}

void VulkanRenderer::GetVkImageViews() {
    _ImageViews.resize(_Images.size());
    for (int i = 0; i < _Images.size(); i++) {
        vk::ImageViewCreateInfo info;
        info.setImage(_Images[i]);     // Texture
        info.setFormat(_SupportInfo.format.format);
        info.setViewType(vk::ImageViewType::e2D);
        vk::ImageSubresourceRange range;
        range.setBaseMipLevel(0);
        range.setLevelCount(1);
        range.setLayerCount(1);
        range.setBaseArrayLayer(0);
        range.setAspectMask(vk::ImageAspectFlagBits::eColor);
        info.setSubresourceRange(range);
        vk::ComponentMapping mapping;
        info.setComponents(mapping);
        _ImageViews[i] = _VkDevice.createImageView(info);
    }
}

vk::ShaderModule VulkanRenderer::CreateShaderModule(const char* shader_file) {
    // Load file by binary
    std::ifstream file(shader_file, std::ios::binary | std::ios::in);
    std::vector<char> content((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
    file.close();

    vk::ShaderModuleCreateInfo info;
    info.setCodeSize(content.size())
        .setPCode((uint32_t*)content.data());

    return _VkDevice.createShaderModule(info);
}

/*
    Vulkan Buffer
*/
MemRequiredInfo VulkanRenderer::QueryMemReqInfo(vk::Buffer buf, vk::MemoryPropertyFlags flag) {
    CHECK(_VkPhyDevice);
    CHECK(_VkDevice);
    
    MemRequiredInfo info;
    vk::PhysicalDeviceMemoryProperties property = _VkPhyDevice.getMemoryProperties();
    vk::MemoryRequirements requirement = _VkDevice.getBufferMemoryRequirements(buf);
    info.size = requirement.size;

    for (uint32_t i = 0; i < property.memoryTypeCount; i++) {
        if (requirement.memoryTypeBits & (1 << i) &&
            property.memoryTypes[i].propertyFlags & (flag)) {
            info.index = i;
        }
    }
    return info;
}

MemRequiredInfo VulkanRenderer::QueryImgReqInfo(vk::Image image, vk::MemoryPropertyFlags flag) {
    CHECK(_VkPhyDevice);
    CHECK(_VkDevice);

    MemRequiredInfo info;
    vk::PhysicalDeviceMemoryProperties property = _VkPhyDevice.getMemoryProperties();
    vk::MemoryRequirements requirement = _VkDevice.getImageMemoryRequirements(image);
    info.size = requirement.size;

    for (uint32_t i = 0; i < property.memoryTypeCount; i++) {
        if (requirement.memoryTypeBits & (1 << i) &&
            property.memoryTypes[i].propertyFlags & (flag)) {
            info.index = i;
        }
    }
    return info;
}

vk::Buffer VulkanRenderer::CreateBuffer(uint64_t size, vk::BufferUsageFlags flag, vk::SharingMode mode) {
    CHECK(_VkDevice);
    vk::BufferCreateInfo info;
    info.setSharingMode(mode)
        .setQueueFamilyIndices(_QueueFamilyProp.graphicsIndex.value())
        .setSize(size)
        .setUsage(flag);
    return _VkDevice.createBuffer(info);
}

vk::DeviceMemory VulkanRenderer::AllocateMemory(MemRequiredInfo memInfo, vk::MemoryPropertyFlags flag) {
    vk::MemoryAllocateInfo info;
    info.setAllocationSize(memInfo.size)
        .setMemoryTypeIndex(memInfo.index);
    return _VkDevice.allocateMemory(info);
}

void VulkanRenderer::CopyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size) {
    vk::CommandBuffer transformCmdBuffer = AllocateCmdBuffer();
    CHECK(transformCmdBuffer);

    vk::BufferCopy regin;
    regin.setSize(size)
        .setSrcOffset(0)
        .setDstOffset(0);
    transformCmdBuffer.copyBuffer(src, dst, regin);
    transformCmdBuffer.end();

    EndCmdBuffer(transformCmdBuffer);
}

void VulkanRenderer::UpLoadMeshes(Mesh& mesh) {
    CHECK(mesh);

    //Allocate vertex buffer
    vk::DeviceSize size = mesh.vertices.size() * sizeof(Vertex);
    vk::Buffer tempBuffer = CreateBuffer(size,
        vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive);
    MemRequiredInfo tempMemInfo = QueryMemReqInfo(tempBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible |
        vk::MemoryPropertyFlagBits::eHostCoherent);
    vk::DeviceMemory tempMemory = AllocateMemory(tempMemInfo,
        vk::MemoryPropertyFlagBits::eHostVisible |
        vk::MemoryPropertyFlagBits::eHostCoherent);

    CHECK(mesh.vertexBuffer.buffer);
    CHECK(mesh.vertexBuffer.memory);

    mesh.vertexBuffer.buffer = CreateBuffer(size, vk::BufferUsageFlagBits::eTransferDst |
        vk::BufferUsageFlagBits::eVertexBuffer, vk::SharingMode::eExclusive);
    MemRequiredInfo memInfo = QueryMemReqInfo(mesh.vertexBuffer.buffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
    mesh.vertexBuffer.memory = AllocateMemory(memInfo, vk::MemoryPropertyFlagBits::eDeviceLocal);
    CHECK(deviceBuffer);
    CHECK(deviceMemory);

    _VkDevice.bindBufferMemory(tempBuffer, tempMemory, 0);
    _VkDevice.bindBufferMemory(mesh.vertexBuffer.buffer, mesh.vertexBuffer.memory, 0);

    void* data = _VkDevice.mapMemory(tempMemory, 0, size);
    memcpy(data, mesh.vertices.data(), size);
    _VkDevice.unmapMemory(tempMemory);

    CopyBuffer(tempBuffer, mesh.vertexBuffer.buffer, size);
    
    // Free mem
    _VkDevice.destroyBuffer(tempBuffer);
    _VkDevice.freeMemory(tempMemory);
}

vk::Format VulkanRenderer::FindSupportedFormat(const std::vector<vk::Format>& candidates,
    vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
    for (vk::Format format : candidates) {
        vk::FormatProperties props;
        _VkPhyDevice.getFormatProperties(format, &props);
        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
        else {
            WARN("Find Supported Format Failed...");
        }
    }

    return vk::Format();
}

void VulkanRenderer::TransitionImageLayout(vk::Image image, vk::Format format,
    vk::ImageLayout oldLayout, vk::ImageLayout newLayout) {

    vk::CommandBuffer cmdBuf = AllocateCmdBuffer();
    CHECK(cmdBuf);

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags dstStage;
    vk::ImageMemoryBarrier barrier;
    vk::ImageSubresourceRange subRange;
    subRange.setAspectMask(vk::ImageAspectFlagBits::eColor)
        .setBaseArrayLayer(0)
        .setBaseMipLevel(0)
        .setLayerCount(1)
        .setLevelCount(1);
    barrier.setOldLayout(oldLayout)
        .setNewLayout(newLayout)
        .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setImage(image)
        .setSubresourceRange(subRange);

    if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        barrier.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eDepth);
        if (IsContainStencilComponent(format)) {
            barrier.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil);
        }
        else {
            barrier.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eDepth);
        }
    }
    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
        barrier.setSrcAccessMask(vk::AccessFlagBits::eNone)
            .setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        dstStage = vk::PipelineStageFlagBits::eTransfer;
    }
    else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead);
        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        dstStage = vk::PipelineStageFlagBits::eFragmentShader;
    }
    else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        barrier.setSrcAccessMask(vk::AccessFlagBits::eNone);
        barrier.setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentRead |
            vk::AccessFlagBits::eDepthStencilAttachmentWrite);

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        dstStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
    }
    else {
        throw std::runtime_error("Transfer ImageLayout Failed...");
    }

    cmdBuf.pipelineBarrier(sourceStage, dstStage, vk::DependencyFlagBits::eByRegion, 0, nullptr, 0, nullptr, 1, &barrier);
    cmdBuf.end();
    EndCmdBuffer(cmdBuf);
}

/*
*  Pipeline
*  All Stage
*/
vk::PipelineShaderStageCreateInfo VulkanRenderer::InitShaderStageCreateInfo(vk::ShaderStageFlagBits stage, vk::ShaderModule shader_module) {
    vk::PipelineShaderStageCreateInfo info;
    info.setStage(stage)
        .setModule(shader_module)
        .setPName("main");

    return info;
}

vk::PipelineVertexInputStateCreateInfo VulkanRenderer::InitVertexInputStateCreateInfo() {
    vk::PipelineVertexInputStateCreateInfo info;
    info.setVertexBindingDescriptionCount(0)
        .setVertexAttributeDescriptionCount(0);

    return info;
}

vk::PipelineInputAssemblyStateCreateInfo VulkanRenderer::InitAssemblyStateCreateInfo(vk::PrimitiveTopology topology) {
    vk::PipelineInputAssemblyStateCreateInfo info;
    info.setTopology(topology)
        .setPrimitiveRestartEnable(VK_FALSE);

    return info;
}

vk::PipelineRasterizationStateCreateInfo VulkanRenderer::InitRasterizationStateCreateInfo(vk::PolygonMode polygonMode) {
    vk::PipelineRasterizationStateCreateInfo info;
    info.setLineWidth(1.0f)
        // No back cull
        .setFrontFace(vk::FrontFace::eClockwise)
        .setCullMode(vk::CullModeFlagBits::eNone)
        // Depth
        .setDepthBiasClamp(0.0f)
        .setDepthBiasEnable(VK_FALSE)
        .setDepthClampEnable(VK_FALSE)
        .setDepthBiasSlopeFactor(0.0f)
        .setDepthBiasConstantFactor(0.0f)
        .setPolygonMode(polygonMode)
        .setRasterizerDiscardEnable(VK_FALSE);

    return info;
}

vk::PipelineMultisampleStateCreateInfo VulkanRenderer::InitMultisampleStateCreateInfo() {
    vk::PipelineMultisampleStateCreateInfo info;
    info.setSampleShadingEnable(VK_FALSE)
        .setMinSampleShading(1.0f)
        .setAlphaToOneEnable(VK_FALSE)
        .setAlphaToCoverageEnable(VK_FALSE)
        .setPSampleMask(nullptr)
        .setRasterizationSamples(vk::SampleCountFlagBits::e1);

    return info;
}

vk::PipelineColorBlendAttachmentState VulkanRenderer::InitColorBlendAttachmentState() {
    vk::PipelineColorBlendAttachmentState info;
    info.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
    info.setBlendEnable(VK_FALSE);

    return info;
}

vk::PipelineLayoutCreateInfo VulkanRenderer::InitPipelineLayoutCreateInfo() {
    vk::PipelineLayoutCreateInfo info;
    info.setSetLayoutCount(0)
        .setPSetLayouts(nullptr)
        .setPushConstantRangeCount(0)
        .setPushConstantRanges(nullptr);

    return info;
}

vk::PipelineDepthStencilStateCreateInfo VulkanRenderer::InitDepthStencilStateCreateInfo() {
    vk::PipelineDepthStencilStateCreateInfo depthStencilInfo;
    depthStencilInfo.setDepthTestEnable(VK_TRUE)    //将新的深度缓冲区与深度缓冲区进行比较，确定是否丢弃
        .setDepthWriteEnable(VK_TRUE)   //测试新深度缓冲区是否应该写入
        .setDepthCompareOp(vk::CompareOp::eLess)
        .setDepthBoundsTestEnable(VK_FALSE)
        .setMinDepthBounds(0.0f)
        .setMaxDepthBounds(1.0f)
        .setStencilTestEnable(VK_FALSE);

    return depthStencilInfo;
}

/*
    Image
*/
vk::Image VulkanRenderer::CreateImage(vk::Format format, vk::ImageUsageFlags usage, vk::Extent3D extent) {
    CHECK(_VkDevice);

    vk::ImageCreateInfo info;
    info.setImageType(vk::ImageType::e2D)
        .setMipLevels(1)
        .setFormat(format)
        .setExtent(extent)
        .setArrayLayers(1)
        .setTiling(vk::ImageTiling::eOptimal)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setSharingMode(vk::SharingMode::eExclusive)
        .setUsage(usage)
        .setSamples(vk::SampleCountFlagBits::e1);
    return _VkDevice.createImage(info);
}

vk::ImageView VulkanRenderer::CreateImageView(vk::Format format, vk::Image image, vk::ImageAspectFlags aspect) {
    CHECK(_VkDevice);

    vk::ImageSubresourceRange subresourceRange;
    subresourceRange.setBaseMipLevel(0)
        .setLevelCount(1)
        .setBaseArrayLayer(0)
        .setLayerCount(1)
        .setAspectMask(aspect);
    
    vk::ImageViewCreateInfo info;
    info.setViewType(vk::ImageViewType::e2D)
        .setFormat(format)
        .setImage(image)
        .setSubresourceRange(subresourceRange);
    return _VkDevice.createImageView(info);
}
