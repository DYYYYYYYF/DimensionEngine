#include "VulkanRenderer.hpp"
#include "PipelineBuilder.hpp"

using namespace renderer;

VulkanRenderer::VulkanRenderer() {
    INFO("Use Vulkan Renderer.");
}

VulkanRenderer::~VulkanRenderer() {

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
    AllocateCmdBuffer();
    CreateFrameBuffers();
    InitSyncStructures();
    CreatePipeline();

    return true; 
}
void VulkanRenderer::Release(){

    _VkDevice.destroyFence(_RenderFence);
    _VkDevice.destroySemaphore(_PresentSemaphore);
    _VkDevice.destroySemaphore(_RenderSemaphore);
    _VkDevice.destroyCommandPool(_VkCmdPool);
    _VkDevice.destroyRenderPass(_VkRenderPass);
    for (auto& frameBuffer : _FrameBuffers) { _VkDevice.destroyFramebuffer(frameBuffer); }
    for (auto& view : _ImageViews) { _VkDevice.destroyImageView(view); }
    for (auto& image : _Images) { _VkDevice.destroyImage(image); }
    _VkDevice.destroySwapchainKHR(_SwapchainKHR);
    _VkDevice.destroy();
    _VkInstance.destroySurfaceKHR(_SurfaceKHR);
    _VkInstance.destroy();

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
    std::vector<const char*> extensionNames;
    unsigned int extensionsCount;
    SDL_Vulkan_GetInstanceExtensions(_Window, &extensionsCount, nullptr);
    extensionNames.resize(extensionsCount);
    SDL_Vulkan_GetInstanceExtensions(_Window, &extensionsCount, extensionNames.data());

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
    info.setPpEnabledExtensionNames(extensionNames.data())
        .setPEnabledLayerNames(layers)
        .setEnabledExtensionCount(extensionsCount)
        .setEnabledLayerCount((uint32_t)layers.size());

#ifdef DEBUG
    std::cout << "Added Extensions:\n";
    for (auto& extension : extensionNames) std::cout << extension << std::endl;
    std::cout << "Added Layers:\n";
    for (auto& layer : layers) std::cout << layer << std::endl;
#endif

    _VkInstance = vk::createInstance(info);
}

void VulkanRenderer::PickupPhyDevice() {
    std::vector<vk::PhysicalDevice> physicalDevices = _VkInstance.enumeratePhysicalDevices();
    std::cout << "\nUsing GPU:  " << physicalDevices[0].getProperties().deviceName << std::endl;    //输出显卡名称
    _VkPhyDevice = physicalDevices[0];
}

void VulkanRenderer::CreateSurface(){
    CHECK(_Window); 
    CHECK(_VkInstance);

    VkSurfaceKHR surface;
    if (!SDL_Vulkan_CreateSurface(_Window, _VkInstance, &surface)){
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

    vk::RenderPassCreateInfo info;
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
    info.setAttachments(attchmentDesc);

    vk::AttachmentReference colRefer;
    colRefer.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
    colRefer.setAttachment(0);

    //深度附件
 // vk::AttachmentDescription depthAttchDesc;
 // depthAttchDesc.setFormat(FindDepthFormat())
 //               .setSamples(vk::SampleCountFlagBits::e1)
 //               .setLoadOp(vk::AttachmentLoadOp::eClear)
 //               .setStoreOp(vk::AttachmentStoreOp::eDontCare)
 //               .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
 //               .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
 //               .setInitialLayout(vk::ImageLayout::eUndefined)
 //               .setFinalLayout(vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal);
 // vk::AttachmentReference depthRef;
 // depthRef.setAttachment(1)
 //         .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
 //
    vk::SubpassDescription subpassDesc;
    subpassDesc.setColorAttachments(colRefer);
    // subpassDesc.setPDepthStencilAttachment(&depthRef);
    subpassDesc.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);

    // std::array<vk::AttachmentDescription, 2> attachments = {attchmentDesc, depthAttchDesc};
    std::array<vk::AttachmentDescription, 1> attachments = { attchmentDesc };
    info.setSubpassCount(1)
        .setSubpasses(subpassDesc)
        .setAttachmentCount((uint32_t)attachments.size())
        .setAttachments(attachments);

    _VkRenderPass = _VkDevice.createRenderPass(info);
    CHECK(_VkRenderPass);
}

void VulkanRenderer::CreateCmdPool() {
    CHECK(_VkDevice);

    vk::CommandPoolCreateInfo info;
    info.setQueueFamilyIndex(_QueueFamilyProp.graphicsIndex.value())
        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    _VkCmdPool = _VkDevice.createCommandPool(info);

    CHECK(_VkCmdPool);
}

vk::CommandBuffer VulkanRenderer::AllocateCmdBuffer() {
    CHECK(_VkDevice);
    CHECK(_VkCmdPool);
    if (!_VkDevice || !_VkCmdPool) {
        return nullptr;
    }

    vk::CommandBufferAllocateInfo allocte;
    allocte.setCommandPool(_VkCmdPool)
        .setCommandBufferCount(1)
        .setLevel(vk::CommandBufferLevel::ePrimary);

    _VkCmdBuffer = _VkDevice.allocateCommandBuffers(allocte)[0];

    vk::CommandBufferBeginInfo info;
    info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    _VkCmdBuffer.begin(info);

    return _VkCmdBuffer;
}

void VulkanRenderer::CreateFrameBuffers() {
    CHECK(_SwapchainKHR);
    const int nSwapchainCount = (int)_Images.size();
    _FrameBuffers = std::vector<vk::Framebuffer>(nSwapchainCount);

    for (int i = 0; i < nSwapchainCount; ++i) {
        vk::FramebufferCreateInfo info;
        info.setRenderPass(_VkRenderPass)
            .setAttachmentCount(1)
            .setWidth(_SupportInfo.GetWindowWidth())
            .setHeight(_SupportInfo.GetWindowHeight())
            .setLayers(1)
            .setPAttachments(&_ImageViews[i]);
        _FrameBuffers[i] = _VkDevice.createFramebuffer(info);
        CHECK(_FrameBuffers[i]);
    }
}

void VulkanRenderer::InitSyncStructures() {
    vk::FenceCreateInfo FenceInfo;
    FenceInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
    _RenderFence = _VkDevice.createFence(FenceInfo);
    CHECK(_RenderFence);

    vk::SemaphoreCreateInfo SemapInfo;
    _RenderSemaphore = _VkDevice.createSemaphore(SemapInfo);
    CHECK(_RenderSemaphore);

    _PresentSemaphore = _VkDevice.createSemaphore(SemapInfo);
    CHECK(_PresentSemaphore);
}

void VulkanRenderer::CreatePipeline() {

    _VertShader = CreateShaderModule("../shader/triangle_vert.spv");
    _FragShader = CreateShaderModule("../shader/triangle_frag.spv");
    CHECK(_VertShader);
    CHECK(_FragShader);

    INFO("Pipeline Layout");
    vk::PipelineLayoutCreateInfo info = InitPipelineLayoutCreateInfo();
    _PipelineLayout = _VkDevice.createPipelineLayout(info);
    CHECK(_PipelineLayout);

    INFO("Pipeline Shader Stages");
    PipelineBuilder pipelineBuilder;
    pipelineBuilder._ShaderStages.push_back(InitShaderStageCreateInfo(vk::ShaderStageFlagBits::eVertex, _VertShader));
    pipelineBuilder._ShaderStages.push_back(InitShaderStageCreateInfo(vk::ShaderStageFlagBits::eFragment, _FragShader));

    INFO("Pipeline Vertex");
    pipelineBuilder._VertexInputInfo = InitVertexInputStateCreateInfo();
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

    _Pipeline = pipelineBuilder.BuildPipeline(_VkDevice, _VkRenderPass);
}


void VulkanRenderer::DrawPerFrame() {
    if (_VkDevice.waitForFences(1, &_RenderFence, true, 1000000000) != vk::Result::eSuccess) {
        return;
    }
    if (_VkDevice.resetFences(1, &_RenderFence) != vk::Result::eSuccess) {
        return;
    }

    auto res = _VkDevice.acquireNextImageKHR(_SwapchainKHR, 1000000000, _PresentSemaphore, nullptr);
    uint32_t nSwapchainImageIndex = res.value;

    _VkCmdBuffer.reset();

    vk::CommandBufferBeginInfo info;
    info.setPInheritanceInfo(nullptr)
        .setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse);

    _VkCmdBuffer.begin(info);

    vk::ClearValue ClearValue;
    vk::ClearColorValue color = std::array<float, 4>{0.5f, 0.5f, 0.5f, 1.0f};
    ClearValue.setColor(color);

    vk::RenderPassBeginInfo rpInfo;
    vk::Extent2D extent = _SupportInfo.extent;
    rpInfo.setRenderPass(_VkRenderPass)
        .setRenderArea(vk::Rect2D({ 0, 0 }, extent))
        .setFramebuffer(_FrameBuffers[nSwapchainImageIndex])
        .setClearValueCount(1)
        .setClearValues(ClearValue);

    _VkCmdBuffer.beginRenderPass(rpInfo, vk::SubpassContents::eInline);

    _VkCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _Pipeline);
    _VkCmdBuffer.draw(3, 1, 0, 0);

    _VkCmdBuffer.endRenderPass();
    _VkCmdBuffer.end();

    vk::PipelineStageFlags WaitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submit;
    submit.setWaitSemaphoreCount(1)
        .setWaitSemaphores(_PresentSemaphore)
        .setSignalSemaphoreCount(1)
        .setSignalSemaphores(_RenderSemaphore)
        .setCommandBufferCount(1)
        .setCommandBuffers(_VkCmdBuffer)
        .setWaitDstStageMask(WaitStage);

    if (_Queue.GraphicsQueue.submit(1, &submit, _RenderFence) != vk::Result::eSuccess) {
        return;
    }
    vk::PresentInfoKHR PresentInfo;
    PresentInfo.setSwapchainCount(1)
        .setSwapchains(_SwapchainKHR)
        .setWaitSemaphoreCount(1)
        .setWaitSemaphores(_RenderSemaphore)
        .setImageIndices(nSwapchainImageIndex);

    if (_Queue.GraphicsQueue.presentKHR(PresentInfo) != vk::Result::eSuccess) {
        return;
    }

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

#ifdef DEBUG 
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
        .setCullMode(vk::CullModeFlagBits::eBack)
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