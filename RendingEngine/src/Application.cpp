#include "Application.hpp"
#include "GLFW/glfw3.h"
#include "tiny_obj_loader.h"
#include <cstring>
#include <iostream>
#include <stb_image.h>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

const uint32_t WIDTH = 1200;
const uint32_t HEIGHT = 800;
// const char* texturePath = "/Users/uncled/Desktop/CMakeFiles/ComputerGraph/TestVulkanDemo/texture/vase_bump.png";
 const char* texturePath = "/Users/uncled/Desktop/CMakeFiles/ComputerGraph/TestVulkanDemo/texture/room.png";
// const char* texturePath = "/Users/uncled/Desktop/CMakeFiles/ComputerGraph/TestVulkanDemo/texture/ship.png";
// const char* texturePath = "/Users/uncled/Desktop/CMakeFiles/ComputerGraph/TestVulkanDemo/texture/rainbow.jpeg";
// const char* texturePath;

uint32_t glfwExtensionCount = 0;
uint32_t extensionCount = 0;

#define CHECK_NULL(expr) \
        if(!(expr)){     \
            throw std::runtime_error(#expr "is nullptr");   \
        }

float scale_callback = 0.5;
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset){
    if(scale_callback + yoffset * 0.02 >= 0.1)
        scale_callback += yoffset*0.02;
}

void TriangleApplication::initWindow(){
    //创建窗口
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); 
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetScrollCallback(window, scroll_callback);
}

void TriangleApplication::initVulkan(){
    std::cout << "exec init..." << std::endl;

    ubo.model = {1,0,0,0,
                 0,1,0,0,
                 0,0,1,0,
                 0,0,0,1};
    ubo.view = {1,0,0,0,
                 0,1,0,0,
                 0,0,1,0,
                 0,0,0,1};
    ubo.projective = {1,0,0,0,
                      0,1,0,0,
                      0,0,1,0,
                      0,0,0,1};
    
    glm::mat4 trans = glm::scale(glm::mat4(1), glm::vec3(TriangleApplication::keyBoardScoll.scale, TriangleApplication::keyBoardScoll.scale, TriangleApplication::keyBoardScoll.scale));
    ubo.model = trans;
    ubo.model = glm::rotate(trans, glm::radians(float(90)), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = camera.getViewMatrix();
    ubo.projective = glm::perspective(glm::radians(45.0f), WIDTH / (float) HEIGHT, 0.1f, 1000.0f);
    ubo.projective[1][1] *= -1;

    std::cout << "size: " << std::endl;
    std::cout << "\tmat4: " << sizeof(glm::mat4) << "\n\tvec3: " << sizeof(glm::vec3) << "\n\tfloat: " << sizeof(float) << std::endl;
        //程序信息：程序名、引擎名称、版本号、API版本等...
    vk::ApplicationInfo appInfo;
    appInfo.pApplicationName = "Triangle";

    //创建Surface、Instance 、PhysicalDevice、Device
    instance = createInstance();
    CHECK_NULL(instance);
    surface = createSurface();
    CHECK_NULL(surface);
    phyDevice = pickupPhysicalDevice();
    CHECK_NULL(phyDevice);
    queueIndices = queryPhysicalDevice();

    std::cout << "indices:\n";
    std::cout << "\t" << queueIndices.graphicsIndices.value() << std::endl;
    std::cout << "\t" << queueIndices.presentIndices.value() << std::endl;

    device = createDeive();
    CHECK_NULL(device);

    graphicQueue = device.getQueue(queueIndices.graphicsIndices.value(),0); 
    presentQueue = device.getQueue(queueIndices.presentIndices.value(),0);
    CHECK_NULL(graphicQueue);
    CHECK_NULL(presentQueue);

    int w, h;
    glfwGetWindowSize(window, &w, &h);
    supportInfo = querySwapchainSupport(w, h);
    swapChain = createSwapchain();
    CHECK_NULL(swapChain);
    images = device.getSwapchainImagesKHR(swapChain);
    views = createImageViews();
    
    renderPass = createRenderPass();
    CHECK_NULL(renderPass);

    cmdPool = createCmdPool();
    CHECK_NULL(cmdPool);
    cmdBuffer = alloateCmdBuffer();
    CHECK_NULL(cmdBuffer);

    createDepthResource();

    framebuffers = createFramebuffer();
    for(auto &fbuffer: framebuffers){
        CHECK_NULL(fbuffer);
    }

    imageAvaliableSem = createSemaphore();
    renderFinishSem = createSemaphore();
    CHECK_NULL(imageAvaliableSem);
    CHECK_NULL(renderFinishSem);

    fence = createFence();
    CHECK_NULL(fence);

    loadTexture();
    createTextureImageView();

    // models.push_back(loadModel(shipObj));
     models.push_back(loadModel(roomObj));
    //models.push_back(loadModel(objFile));
    createVertexBuf(models[0]);
    createIndexBuf(models[0]);
    //models.push_back(loadModel(roomObj));
    //createVertexBuf(models[1]);
    //createIndexBuf(models[1]);
    createUniformBuf();
    createTextureSample();
    // phyDevice.getFeatures().setSampleRateShading(true);
    // phyDevice.getFeatures().setFillModeNonSolid(true);

    uint32_t count;
    vk::Result res = phyDevice.enumerateDeviceExtensionProperties(nullptr, &count, nullptr);
    if(res != vk::Result::eSuccess){
        std::runtime_error("get DeviceExtensions Failed...");
    }
    std::vector<vk::ExtensionProperties> avaliableExtensions;
    avaliableExtensions.resize(count);
    res = phyDevice.enumerateDeviceExtensionProperties(nullptr, &count, &avaliableExtensions[0]);
    std::cout << "DeviceAvaliableExtensions\n";
    for(int i=0; i<count; i++){
        std:: cout << "\t" << avaliableExtensions[i].extensionName << std::endl;
    }
    std::cout << "Memory Properties: \n\t" << 
        phyDevice.getProperties().limits.minUniformBufferOffsetAlignment << std::endl;

    //PipelineLayout
    }   

vk::Instance TriangleApplication::createInstance(){
    //get extensions
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    vk::InstanceCreateInfo cInfo;
    //validation layers
    std::array<const char*, 1> layers{"VK_LAYER_KHRONOS_validation"};

    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    //加入扩展--MacOS需要的(PS：不知道为什么Mac会需要)
    glfwExtensions[glfwExtensionCount] = "VK_KHR_get_physical_device_properties2";
    cInfo.setEnabledExtensionCount(++glfwExtensionCount);
    cInfo.setPpEnabledExtensionNames(glfwExtensions);
    cInfo.setPEnabledLayerNames(layers);

    std::cout << "Extensions Name:\n";
    for(auto &extension: extensions){
        std::cout << "\t" << extension.extensionName << std::endl;
    }

    return vk::createInstance(cInfo);
}

vk::SurfaceKHR TriangleApplication::createSurface(){
    VkSurfaceKHR surface;
    if(glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS){
        throw std::runtime_error("failed to create window surface.");
    }
    return surface;
}

vk::PhysicalDevice TriangleApplication::pickupPhysicalDevice(){
    auto physicalDevices = instance.enumeratePhysicalDevices();
    std::cout << "Using GPU:" << physicalDevices[0].getProperties().deviceName << std::endl;    //输出显卡名称
    return physicalDevices[0];
}

TriangleApplication::QueueFamilyIndices TriangleApplication::queryPhysicalDevice(){
    TriangleApplication::QueueFamilyIndices indices;
    auto families = phyDevice.getQueueFamilyProperties();
    uint32_t index = 0;
    for(auto &family:families){
        //选取Graph相关指令
        if(family.queueFlags | vk::QueueFlagBits::eGraphics){
            indices.graphicsIndices = index;
        }
        //选取Surface相关指令
        if(phyDevice.getSurfaceSupportKHR(index, surface)){
            indices.presentIndices = index;
        }
        //如果都找到对应序列索引，直接退出
        if(indices.graphicsIndices && indices.presentIndices) {
            break;
        }
        index++;
    }
    return indices;
}

vk::Device TriangleApplication::createDeive(){
    std::vector<vk::DeviceQueueCreateInfo> dqInfo;
    //两者索引值相同，只需要创建一个实体
    if(queueIndices.graphicsIndices.value() == queueIndices.presentIndices.value()){
        vk::DeviceQueueCreateInfo info;
        float priority = 1.0;
        info.setQueuePriorities(priority);      //优先级
        info.setQueueCount(1);          //队列个数
        info.setQueueFamilyIndex(queueIndices.graphicsIndices.value());     //下标  
        dqInfo.push_back(info);        
    } else {
        vk::DeviceQueueCreateInfo info1;
        float priority = 1.0;
        info1.setQueuePriorities(priority); 
        info1.setQueueCount(1);
        info1.setQueueFamilyIndex(queueIndices.graphicsIndices.value());

        vk::DeviceQueueCreateInfo info2;
        info2.setQueuePriorities(priority);
        info2.setQueueCount(1);
        info2.setQueueFamilyIndex(queueIndices.presentIndices.value());

        dqInfo.push_back(info1);       
        dqInfo.push_back(info2);        
    }
    vk::DeviceCreateInfo dInfo;
    dInfo.setQueueCreateInfos(dqInfo);
    std::array<const char*,2> extensions{"VK_KHR_portability_subset",
                                          VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    dInfo.setPEnabledExtensionNames(extensions);
    return phyDevice.createDevice(dInfo);
}

vk::SwapchainKHR TriangleApplication::createSwapchain(){
    vk::SwapchainCreateInfoKHR scInfo;
    scInfo.setImageColorSpace(supportInfo.format.colorSpace);
    scInfo.setImageFormat(supportInfo.format.format);
    scInfo.setImageExtent(supportInfo.extent);
    scInfo.setMinImageCount(supportInfo.imageCount);
    scInfo.setPresentMode(supportInfo.presnetMode);
    scInfo.setPreTransform(supportInfo.capabilities.currentTransform);
    
    if(queueIndices.graphicsIndices.value() == queueIndices.presentIndices.value()){
        scInfo.setQueueFamilyIndices(queueIndices.graphicsIndices.value());
        scInfo.setImageSharingMode(vk::SharingMode::eExclusive);
    } else {
        std::array<uint32_t,2> index{queueIndices.graphicsIndices.value(),
                                     queueIndices.presentIndices.value()};
        scInfo.setQueueFamilyIndices(index);
        scInfo.setImageSharingMode(vk::SharingMode::eConcurrent);
    }

    scInfo.setClipped(true);
    scInfo.setSurface(surface);
    scInfo.setImageArrayLayers(1);
    scInfo.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
    scInfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);

    return device.createSwapchainKHR(scInfo);
}

TriangleApplication::SwapchainSupport TriangleApplication::querySwapchainSupport(int w, int h){
    SwapchainSupport supportInfo;
    supportInfo.capabilities = phyDevice.getSurfaceCapabilitiesKHR(surface);
    auto formats = phyDevice.getSurfaceFormatsKHR(surface);
    for(auto &format: formats){
        if(format.format == vk::Format::eR8G8B8A8Srgb || 
           format.format == vk::Format::eB8G8R8A8Srgb || 
           format.format == vk::Format::eR8G8B8A8Unorm){
            supportInfo.format = format;
        }
    }
    supportInfo.extent.width = std::clamp<uint32_t>(w, 
                            supportInfo.capabilities.minImageExtent.width,
                            supportInfo.capabilities.maxImageExtent.width);
    supportInfo.extent.height = std::clamp<uint32_t>(h, 
                            supportInfo.capabilities.minImageExtent.height,
                            supportInfo.capabilities.maxImageExtent.height);
    supportInfo.imageCount = std::clamp<uint32_t>(2,
                            supportInfo.capabilities.minImageCount,
                            supportInfo.capabilities.maxImageCount);
    supportInfo.presnetMode = vk::PresentModeKHR::eFifo;    //默认值--先进先出绘制图像
    auto presentModes = phyDevice.getSurfacePresentModesKHR(surface);
    for(auto& mode:presentModes){
        if(mode == vk::PresentModeKHR::eMailbox){
            supportInfo.presnetMode = mode;
        }
    }
    return supportInfo;
}

std::vector<vk::ImageView> TriangleApplication::createImageViews(){
    std::vector<vk::ImageView> views(images.size());
    for(int i=0; i<images.size(); i++){
        vk::ImageViewCreateInfo info;
        info.setImage(images[i]);     //纹理贴图
        info.setFormat(supportInfo.format.format);
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
        views[i] = device.createImageView(info);
    }
    return views;
}

vk::Pipeline TriangleApplication::createPipeline(vk::ShaderModule vextexShader, vk::ShaderModule fragShader){
    vk::GraphicsPipelineCreateInfo info;
    
    //shader configuration
    std::array<vk::PipelineShaderStageCreateInfo,2> shaderStageInfos;  
    shaderStageInfos[0].setModule(vextexShader);
    shaderStageInfos[0].setStage(vk::ShaderStageFlagBits::eVertex);
    shaderStageInfos[0].setPName("main");
    shaderStageInfos[1].setModule(fragShader);
    shaderStageInfos[1].setStage(vk::ShaderStageFlagBits::eFragment);
    shaderStageInfos[1].setPName("main");
    info.setStages(shaderStageInfos);
    //Vertex Input  -- 传入顶点 ，这里的顶点全部定义在GPU-Shader中
    vk::PipelineVertexInputStateCreateInfo vertexInfo;
    auto bindingDescription = Vertex::GetBindingDescription();
    auto attributeDescription = Vertex::GetAttributeDescription();
    vertexInfo.setVertexAttributeDescriptions(attributeDescription)
              .setVertexBindingDescriptions(bindingDescription);
    info.setPVertexInputState(&vertexInfo);
    //Input Assenmbly
    vk::PipelineInputAssemblyStateCreateInfo assemblyInfo;
    assemblyInfo.setPrimitiveRestartEnable(false)
                .setTopology(vk::PrimitiveTopology::eTriangleList); //转成三角形
    info.setPInputAssemblyState(&assemblyInfo);

    //layout
    
    info.setLayout(layout);
    //viewport & Scissor
    vk::PipelineViewportStateCreateInfo viewportInfo;
    vk::Viewport viewport(0, 0,
                          supportInfo.extent.width, supportInfo.extent.height,
                          0, 1);
    vk::Rect2D scissor({0,0}, supportInfo.extent);
    viewportInfo.setViewports(viewport)     //视口
                .setScissors(scissor);      //裁剪
                
    info.setPViewportState(&viewportInfo);

    //Rasterization
    vk::PipelineRasterizationStateCreateInfo rasterInfo;
    rasterInfo.setRasterizerDiscardEnable(false)
              .setDepthClampEnable(false)     //true --> 避免产生空洞
              .setDepthBiasEnable(true)    //true --> 纹理相关
              .setLineWidth(1)
              .setCullMode(vk::CullModeFlagBits::eNone) //不剔除面
              .setPolygonMode(polygonMode)  //填充
              .setFrontFace(vk::FrontFace::eCounterClockwise);
    info.setPRasterizationState(&rasterInfo);

    //Multisample -- 采样，抗锯齿
    vk::PipelineMultisampleStateCreateInfo musltisample;
    musltisample.setSampleShadingEnable(false)  //关闭抗锯齿
                .setRasterizationSamples(vk::SampleCountFlagBits::e1);  //自己本身
    info.setPMultisampleState(&musltisample);

    //DepthStencil
    vk::PipelineDepthStencilStateCreateInfo depthStencilInfo;
    depthStencilInfo.setDepthTestEnable(VK_TRUE)    //将新的深度缓冲区与深度缓冲区进行比较，确定是否丢弃
                    .setDepthWriteEnable(VK_TRUE)   //测试新深度缓冲区是否应该写入
                    .setDepthCompareOp(vk::CompareOp::eLess)
                    .setDepthBoundsTestEnable(VK_FALSE)
                    .setMinDepthBounds(1.0f)
                    .setMaxDepthBounds(1.0f)
                    .setStencilTestEnable(VK_FALSE);

    info.setPDepthStencilState(&depthStencilInfo);

    //Color blend
    vk::PipelineColorBlendStateCreateInfo colorBlend;
    vk::PipelineColorBlendAttachmentState attBlendState;
    attBlendState.setColorWriteMask(vk::ColorComponentFlagBits::eA | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eR );
    colorBlend.setLogicOpEnable(false)
              .setAttachments(attBlendState);
    info.setPColorBlendState(&colorBlend);

    //RenderPass
    info.setRenderPass(renderPass);

    //Create Pipeline
    auto result = device.createGraphicsPipeline(nullptr, info);

    if(result.result != vk::Result::eSuccess){
        throw std::runtime_error("Pipeline create failed");
    }
    return result.value;
}

vk::ShaderModule TriangleApplication::createShaderModule(const char* filename){
    //二进制格式读取文件
    std::ifstream file(filename, std::ios::binary | std::ios::in);
    std::vector<char> content((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
    file.close();

    vk::ShaderModuleCreateInfo info;
    info.pCode = (uint32_t *)content.data();
    info.setCodeSize(content.size());
    std::cout << "Size: " << content.size() << std::endl;

    modules.push_back(device.createShaderModule(info));
    return modules.back();    
}

vk::PipelineLayout TriangleApplication::createLayout(){
    vk::PipelineLayoutCreateInfo layout;
    layout.setSetLayouts(uniformDesLayout)
        .setSetLayoutCount(1);
    return device.createPipelineLayout(layout);
}

vk::RenderPass TriangleApplication::createRenderPass(){
    vk::RenderPassCreateInfo info;
    //颜色附件
    vk::AttachmentDescription attchmentDesc;
    attchmentDesc.setSamples(vk::SampleCountFlagBits::e1)
                 .setLoadOp(vk::AttachmentLoadOp::eClear)
                 .setStoreOp(vk::AttachmentStoreOp::eStore)
                 .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                 .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                 .setFormat(supportInfo.format.format)
                 .setInitialLayout(vk::ImageLayout::eUndefined)
                 .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);
    info.setAttachments(attchmentDesc);
    
    vk::SubpassDescription subpassDesc;
    vk::AttachmentReference colRefer;
    colRefer.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
    colRefer.setAttachment(0);

       //深度附件
    vk::AttachmentDescription depthAttchDesc;
    depthAttchDesc.setFormat(findDepthForamt())
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

    subpassDesc.setColorAttachments(colRefer);
    subpassDesc.setPDepthStencilAttachment(&depthRef);
    subpassDesc.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);

    std::array<vk::AttachmentDescription, 2> attachments = {attchmentDesc, depthAttchDesc};
    info.setSubpassCount(1)
        .setSubpasses(subpassDesc)
        .setAttachmentCount(attachments.size())
        .setAttachments(attachments);
        
    return device.createRenderPass(info);                
}

std::vector<vk::Framebuffer> TriangleApplication::createFramebuffer(){
    std::vector<vk::Framebuffer> buffers;
    for(int i=0; i<views.size(); i++){
        std::array<vk::ImageView, 2> attchments = {views[i], depthImageView};
        vk::FramebufferCreateInfo cinfo;
        cinfo.setRenderPass(renderPass);
        cinfo.setAttachmentCount(attchments.size());
        cinfo.setWidth(supportInfo.extent.width);
        cinfo.setHeight(supportInfo.extent.height);
        cinfo.setPAttachments(attchments.data());
        cinfo.setLayers(1);

        buffers.push_back(device.createFramebuffer(cinfo));
    }

    return buffers;
}

vk::CommandPool TriangleApplication::createCmdPool(){
    vk::CommandPoolCreateInfo info;
    info.setQueueFamilyIndex(queueIndices.graphicsIndices.value())
        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    return device.createCommandPool(info);
}

vk::CommandBuffer TriangleApplication::alloateCmdBuffer(){
    vk::CommandBuffer cmdBuf;
    vk::CommandBufferAllocateInfo allocte;
    allocte.setCommandPool(cmdPool)
           .setCommandBufferCount(1)
           .setLevel(vk::CommandBufferLevel::ePrimary);
    
    cmdBuf = device.allocateCommandBuffers(allocte)[0];

    vk::CommandBufferBeginInfo info;
    info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    cmdBuf.begin(info);
    return cmdBuf;
}

void TriangleApplication::endCmdBuffer(vk::CommandBuffer cmdBuf){
    vk::SubmitInfo submitInfo;
    submitInfo.setCommandBuffers(cmdBuf);

    graphicQueue.submit(submitInfo);
    device.waitIdle();
    device.freeCommandBuffers(cmdPool, cmdBuf);
}

void TriangleApplication::recordCmd(vk::CommandBuffer buf, vk::Framebuffer fbur, vk::Pipeline pipeline, TriangleApplication::ObjModel model){
    vk::CommandBufferBeginInfo info;
    info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        
    if(buf.begin(&info) != vk::Result::eSuccess){
        throw std::runtime_error("command buffer record failed");
    }

    vk::RenderPassBeginInfo renderPassBegin;
    vk::ClearColorValue cvalue(std::array<float,4>{0.1, 0.1, 0.1, 1});
    std::array<vk::ClearValue, 2> clearValues;
    clearValues[0].setColor(cvalue);
    clearValues[1].setDepthStencil({1.0, 0});
    renderPassBegin.setRenderPass(renderPass)
                   .setRenderArea(vk::Rect2D({0,0}, supportInfo.extent))
                   .setClearValueCount(clearValues.size())
                   .setPClearValues(clearValues.data())
                   .setFramebuffer(fbur);
                
    buf.beginRenderPass(renderPassBegin, vk::SubpassContents::eInline);

    buf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

    vk::DeviceSize size = 0;
    buf.bindVertexBuffers(0, deviceBuffer, size);
    buf.bindIndexBuffer(indexBuf, 0, vk::IndexType::eUint32);
    buf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, 0, 1, &descSets, 0, nullptr);
    

    // buf.draw(vertices.size(), 1, 0, 0);
    buf.drawIndexed(model.indices.size(), 1, 0, 0, 0);

    buf.endRenderPass();
    buf.end();
}

void TriangleApplication::Render(vk::Pipeline curPipeline, TriangleApplication::ObjModel model){
    device.resetFences(fence);
    auto result = device.acquireNextImageKHR(swapChain, std::numeric_limits<uint64_t>::max(), imageAvaliableSem, nullptr);
    // if(result.result != vk::Result::eSuccess){
    //     throw std::runtime_error("acquired image failed");
    // }
    uint32_t imageIndex = result.value;

    cmdBuffer.reset();
    recordCmd(cmdBuffer, framebuffers[imageIndex], curPipeline, model);

    vk::SubmitInfo submitInfo;
    vk::PipelineStageFlags flags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    submitInfo.setCommandBuffers(cmdBuffer)
              .setSignalSemaphores(renderFinishSem)
              .setWaitSemaphores(imageAvaliableSem)
              .setWaitDstStageMask(flags);
    graphicQueue.submit(submitInfo, fence);

    vk::PresentInfoKHR presentInfo;
    presentInfo.setImageIndices(imageIndex)
               .setSwapchains(swapChain)
               .setWaitSemaphores(renderFinishSem);
    if(presentQueue.presentKHR(presentInfo) != vk::Result::eSuccess){
    
    }

    if(device.waitForFences(fence, true, std::numeric_limits<uint64_t>::max()) != vk::Result::eSuccess){
        throw std::runtime_error("wait fence failed");
    }

}

vk::Semaphore TriangleApplication::createSemaphore(){
    vk::SemaphoreCreateInfo info;
    return device.createSemaphore(info);
}

vk::Fence TriangleApplication::createFence(){
    vk::FenceCreateInfo info;
    info.setFlags(vk::FenceCreateFlagBits::eSignaled);
    return device.createFence(info);
}

void TriangleApplication::WaitIdle(){
    device.waitIdle();
}

vk::Buffer TriangleApplication::createBuffer(vk::BufferUsageFlags flag, vk::SharingMode mode, uint64_t size){
    vk::BufferCreateInfo info;
    info.setSharingMode(mode)
        .setQueueFamilyIndices(queueIndices.graphicsIndices.value())
        .setSize(size)
        .setUsage(flag);
    return device.createBuffer(info);
}

vk::DeviceMemory TriangleApplication::allocateMemory(vk::Buffer buffer, vk::MemoryPropertyFlags flag){
    auto memInfo = queryMemReqInfo(buffer,flag);

    vk::MemoryAllocateInfo info;
    info.setAllocationSize(memInfo.size)
        .setMemoryTypeIndex(memInfo.index);
    return device.allocateMemory(info);
}

TriangleApplication::MemRequiredInfo TriangleApplication::queryMemReqInfo(vk::Buffer buf, vk::MemoryPropertyFlags flag){
    MemRequiredInfo info;
    auto property = phyDevice.getMemoryProperties();
    auto requirement = device.getBufferMemoryRequirements(buf);
    info.size = requirement.size;
    
    for(int i=0; i<property.memoryTypeCount; i++){
        if(requirement.memoryTypeBits & (1 << i) &&
           property.memoryTypes[i].propertyFlags & (flag)){
                info.index = i;
           }
    }
    return info;
}

TriangleApplication::MemRequiredInfo TriangleApplication::queryImageReqInfo(vk::Image image, vk::MemoryPropertyFlags flag){
    MemRequiredInfo info;
    auto property = phyDevice.getMemoryProperties();
    auto requirement = device.getImageMemoryRequirements(image);
    info.size = requirement.size;
    
    for(int i=0; i<property.memoryTypeCount; i++){
        if(requirement.memoryTypeBits & (1 << i) &&
           property.memoryTypes[i].propertyFlags & (flag)){
                info.index = i;
           }
    }
    return info;
}

void TriangleApplication::createVertexBuf(TriangleApplication::ObjModel &model){
    vk::DeviceSize size = sizeof(Vertex) * model.vertices.size(); 
    vertexBuffer = createBuffer(vk::BufferUsageFlagBits::eTransferSrc,
                                vk::SharingMode::eExclusive,size);
    vertexMemory = allocateMemory(vertexBuffer, vk::MemoryPropertyFlagBits::eHostVisible |
                                vk::MemoryPropertyFlagBits::eHostCoherent);

    CHECK_NULL(vertexBuffer);
    CHECK_NULL(vertexMemory);

    deviceBuffer = createBuffer(vk::BufferUsageFlagBits::eTransferDst | 
                                vk::BufferUsageFlagBits::eVertexBuffer,
                                vk::SharingMode::eExclusive, size);
    deviceMemory = allocateMemory(deviceBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
    CHECK_NULL(deviceBuffer);
    CHECK_NULL(deviceMemory);

    device.bindBufferMemory(vertexBuffer, vertexMemory, 0);
    device.bindBufferMemory(deviceBuffer, deviceMemory, 0);

    void * data = device.mapMemory(vertexMemory, 0, size);
    memcpy(data, model.vertices.data(), size);
    device.unmapMemory(vertexMemory);

    copyBuffer(vertexBuffer, deviceBuffer, size);
}

void TriangleApplication::createIndexBuf(TriangleApplication::ObjModel &model){
   vk::DeviceSize size = sizeof(uint64_t) * model.indices.size();
   auto tempBuf = createBuffer(vk::BufferUsageFlagBits::eTransferSrc,
                                vk::SharingMode::eExclusive,size);
   auto tempMem = allocateMemory(tempBuf, vk::MemoryPropertyFlagBits::eHostVisible | 
                                vk::MemoryPropertyFlagBits::eHostCoherent);
   device.bindBufferMemory(tempBuf, tempMem, 0);

   indexBuf = createBuffer(vk::BufferUsageFlagBits::eTransferDst |
                                vk::BufferUsageFlagBits::eIndexBuffer,
                                vk::SharingMode::eExclusive, size);
   indexMem = allocateMemory(indexBuf, vk::MemoryPropertyFlagBits::eDeviceLocal);
   device.bindBufferMemory(indexBuf, indexMem, 0);

    void *data = device.mapMemory(tempMem, 0, size);
    memcpy(data, model.indices.data(), size);
    device.unmapMemory(tempMem);

    copyBuffer(tempBuf, indexBuf, size);
    device.destroyBuffer(tempBuf);
    device.freeMemory(tempMem);
}

void TriangleApplication::copyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size){
    vk::CommandBuffer transformCmdBuffer = alloateCmdBuffer();
    CHECK_NULL(transformCmdBuffer);

    vk::BufferCopy regin;
    regin.setSize(size)
         .setSrcOffset(0)
         .setDstOffset(0);
    transformCmdBuffer.copyBuffer(src, dst, regin);
    transformCmdBuffer.end();

    endCmdBuffer(transformCmdBuffer);
}

TriangleApplication::ObjModel TriangleApplication::loadModel(const char* objPaht){
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    TriangleApplication::ObjModel model;
    model.vertices.clear();
    model.indices.clear();

    if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objPaht)){
        std::runtime_error("Error: " + err);
    }

    std::cout << "Shapes num: " << shapes.size() << std::endl;
    for(int i=0; i<shapes.size(); i++) std::cout << "\tShape name: " << shapes[i].name << std::endl;

    std::cout << "Materials num: " << materials.size() << std::endl;
    for(int i=0; i<materials.size(); i++) std::cout << "\tMaterial name: " << materials[i].name << std::endl;

    std::unordered_map<Vertex, uint32_t> uniqueVertices = {};   //消除重复定点数据

    float col = 0.0f;
    for (const auto &shape : shapes) {
        for (const auto &index : shape.mesh.indices) {
            Vertex vertex = {};
            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            // std::cout << "uv: " << attrib.texcoords[index.texcoord_index] << std::endl;
            if(index.texcoord_index >= 0){
                vertex.texCoord = {
                   attrib.texcoords[2 * index.texcoord_index + 0],
                   1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };
            }
            
            vertex.color = {1.0f, 1.0f, 1.0f};
            // vertex.color = {col, col, col};
            vertex.normal = {
                attrib.normals[3 * index.normal_index + 0],
                attrib.normals[3 * index.normal_index + 1],
                attrib.normals[3 * index.normal_index + 2]
            };

            if(uniqueVertices.count(vertex) == 0){
                uniqueVertices[vertex] = static_cast<uint32_t>(model.vertices.size());
                model.vertices.push_back(vertex);
            }
            

            // 纯白： 
            // vertex.color = {1.0f, 1.0f, 1.0f, 1.0f};
            //近白远黑：
            // vertex.color = {vertex.pos.z, 1-vertex.pos.z, 1-vertex.pos.z};

            // vertices.push_back(vertex);
            // indices.push_back(indices.size());
            model.indices.push_back(uniqueVertices[vertex]);
        }
        col += 0.1;
        if(col > 1) col = 0.0f;
    }
    return model;
 }

 vk::DescriptorSetLayout TriangleApplication::createDescriptionSetLayout(){
    vk::DescriptorSetLayoutBinding binding;
    binding.setBinding(0)
        .setDescriptorType(vk::DescriptorType::eUniformBuffer)
        .setDescriptorCount(1)
        .setStageFlags(vk::ShaderStageFlagBits::eVertex);
    vk::DescriptorSetLayoutBinding samplerBinding;
    samplerBinding.setBinding(1)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
        .setPImmutableSamplers(nullptr)
        .setStageFlags(vk::ShaderStageFlagBits::eFragment);

    std::vector<vk::DescriptorSetLayoutBinding> bindings = {binding, samplerBinding};

    vk::DescriptorSetLayoutCreateInfo info;
    info.setBindings(bindings)
        .setBindingCount(bindings.size());
    return device.createDescriptorSetLayout(info);
 }

 void TriangleApplication::createUniformBuf(){
    size_t minUboAlignment = phyDevice.getProperties().limits.minUniformBufferOffsetAlignment;
	size_t dynamicAlignment = sizeof(glm::vec3);
	if (minUboAlignment > 0) {
		dynamicAlignment = (dynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
	}
    uniformBuffer = createBuffer(vk::BufferUsageFlagBits::eUniformBuffer,
                        vk::SharingMode::eExclusive, sizeof(UniformBufferObj));
    uniformMemory = allocateMemory(uniformBuffer,
                        vk::MemoryPropertyFlagBits::eHostVisible | 
                        vk::MemoryPropertyFlagBits::eHostCoherent);
    device.bindBufferMemory(uniformBuffer, uniformMemory, 0);

 }

 void TriangleApplication::updateUniform(TriangleApplication::KeyBoardScoll val){
    //View变换
    ubo.view = camera.getViewMatrix();

    //Model变换
    glm::mat4 trans = ModelMatTools::scale(val.scale);
    ubo.model = trans * ModelMatTools::translate(val.transX, val.transY, val.transZ)
                * ModelMatTools::rotateY(val.rotateY)
                * ModelMatTools::rotateX(val.rotateX)
                * ModelMatTools::rotateZ(val.rotateZ);
    ubo.viewPos = camera.position;
    void *data = device.mapMemory(uniformMemory,0,sizeof(ubo));
    memcpy(data, &ubo, sizeof(ubo));
    device.unmapMemory(uniformMemory);
 }

 vk::DescriptorPool TriangleApplication::createDescPool(){
    std::array<vk::DescriptorPoolSize, 2> PoolSizes;
    PoolSizes[0].setType(vk::DescriptorType::eUniformBuffer)
        .setDescriptorCount(1);
    PoolSizes[1].setType(vk::DescriptorType::eCombinedImageSampler)
        .setDescriptorCount(1);

    vk::DescriptorPoolCreateInfo info;
    info.setPoolSizes(PoolSizes)
        .setPoolSizeCount(PoolSizes.size())
        .setMaxSets(1);

    return device.createDescriptorPool(info);
 }

 vk::DescriptorSet TriangleApplication::createDescSet(){
    vk::DescriptorSetAllocateInfo info;
    info.setDescriptorPool(descPool)
        .setDescriptorSetCount(1)
        .setSetLayouts(uniformDesLayout);

    vk::DescriptorSet res;
    if(device.allocateDescriptorSets(&info, &res) != vk::Result::eSuccess){
        throw std::runtime_error("allocate DescriptiorSets failed...");
    }
    return res;
} 

void TriangleApplication::updateDescSet(){
    vk::DescriptorBufferInfo UniformInfo;
    UniformInfo.setRange(sizeof(UniformBufferObj))
        .setOffset(0)
        .setBuffer(uniformBuffer);
    vk::DescriptorImageInfo imageInfo;
    imageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setImageView(textureImageView)
        .setSampler(textureSampler);
    std::array<vk::WriteDescriptorSet, 2> writeDescSets;;
    writeDescSets[0].setDstSet(descSets)
        .setDstBinding(0)
        .setBufferInfo(UniformInfo)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eUniformBuffer)
        .setDstArrayElement(0);
    writeDescSets[1].setDstSet(descSets)
        .setDstBinding(1)
        .setImageInfo(imageInfo)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
        .setDstArrayElement(0);
    device.updateDescriptorSets(writeDescSets, nullptr);
}

void TriangleApplication::updateKeyBoard(){
    if(glfwGetKey(window, GLFW_KEY_ESCAPE)){
        glfwSetWindowShouldClose(window, true);
    } else if(glfwGetKey(window, GLFW_KEY_W)){
        camera.keyBoardSpeedZ = 10;      //press-W
    } else if(glfwGetKey(window, GLFW_KEY_S)){
        camera.keyBoardSpeedZ = -10;     //press-S
    } else {
        camera.keyBoardSpeedZ = 0;
    }
    if(glfwGetKey(window, GLFW_KEY_A)){
        camera.keyBoardSpeedX = -10;     //press-A
    } else if(glfwGetKey(window, GLFW_KEY_D)){
        camera.keyBoardSpeedX = 10;      //press-D
    } else{
        camera.keyBoardSpeedX = 0;
    }
    if(glfwGetKey(window, GLFW_KEY_Q)){
        camera.keyBoardSpeedY = 5;      //press-Q
    } else if(glfwGetKey(window, GLFW_KEY_E)){
        camera.keyBoardSpeedY = -5;     //press-E
    } else {
        camera.keyBoardSpeedY = 0;
    }
    if(glfwGetKey(window, GLFW_KEY_J)){
        keyBoardScoll.rotateY += 0.1;
    }
    if(glfwGetKey(window, GLFW_KEY_K)){
        keyBoardScoll.rotateY -= 0.1;
    }
    if(glfwGetKey(window, GLFW_KEY_U)){
        keyBoardScoll.rotateX += 0.1;
    }
    if(glfwGetKey(window, GLFW_KEY_I)){
        keyBoardScoll.rotateX -= 0.1;
    }
    if(glfwGetKey(window, GLFW_KEY_LEFT)){
        keyBoardScoll.rotateZ += 0.1;
    }
    if(glfwGetKey(window, GLFW_KEY_RIGHT)){
        keyBoardScoll.rotateZ -= 0.1;
    }

    double posX = 0, posY;
    if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS){
        //press mouse-left
        glfwGetCursorPos(window, &posX, &posY);
        if(keyBoardScoll.mouse.x == 0 && keyBoardScoll.mouse.y == 0){
            keyBoardScoll.mouse.x = posX;
            keyBoardScoll.mouse.y = posY;
        } else {
            double x = posX - keyBoardScoll.mouse.x;
            double y = posY - keyBoardScoll.mouse.y;
            keyBoardScoll.mouse.x = posX;
            keyBoardScoll.mouse.y = posY;

            camera.mouserMovement(-x, -y);
            // keyBoardScoll.rotateX -= glm::dot(glm::vec3(0, 0, x), glm::vec3(0, 0, y)) * 0.01;
        }
    }
    if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE){
        //release mouse-left
        keyBoardScoll.mouse.x = 0;
        keyBoardScoll.mouse.y = 0;
    }

    keyBoardScoll.scale = scale_callback;
}

void TriangleApplication::mainLoop(){
    std::cout << "exec loop..." << std::endl;

    auto vertexShader = TriangleApplication::createShaderModule("./shader/vert.spv");
    auto fragmentShader = TriangleApplication::createShaderModule("./shader/frag.spv");
    auto cartoonShader = TriangleApplication::createShaderModule("./shader/cartoonFrag.spv");
    auto originalShader = TriangleApplication::createShaderModule("./shader/originalFrag.spv");

    //PipelineLayout创建，需要在createPipeline之前初始化
    descPool = createDescPool();
    CHECK_NULL(descPool);
    uniformDesLayout = createDescriptionSetLayout();
    CHECK_NULL(uniformDesLayout);
    descSets = createDescSet();
    CHECK_NULL(descSets);
    layout = createLayout();
    CHECK_NULL(layout);

    pipelines.resize(3);
    pipelines[0] = createPipeline(vertexShader, fragmentShader);
    pipelines[1] = createPipeline(vertexShader, cartoonShader); 
    pipelines[2] = createPipeline(vertexShader, originalShader); 

    updateDescSet();
    
    vk::Pipeline pipeline = pipelines[0];
    //显示窗口
    while(!glfwWindowShouldClose(window)){
        if(glfwGetKey(window, GLFW_KEY_C)) pipeline = pipelines[1];
        if(glfwGetKey(window, GLFW_KEY_X)) pipeline = pipelines[0];
        if(glfwGetKey(window, GLFW_KEY_V)) pipeline = pipelines[2];
        if(glfwGetKey(window, GLFW_KEY_P)){
            if(polygonMode == vk::PolygonMode::eFill){
                polygonMode = vk::PolygonMode::eLine;
            } else {
                polygonMode = vk::PolygonMode::eFill;
            }
            pipelines[0] = createPipeline(vertexShader, fragmentShader);
            pipelines[1] = createPipeline(vertexShader, cartoonShader); 
            pipelines[2] = createPipeline(vertexShader, originalShader); 
        }
    
        updateKeyBoard();
        camera.updateCameraPosition();
        updateUniform(keyBoardScoll);
        glfwPollEvents();
        for(auto m:models){
            Render(pipeline, m);
        }
    }

    WaitIdle();
}

void TriangleApplication::cleanup(){
    std::cout << "exec clean..." << std::endl;

    device.destroySampler(textureSampler);
    device.destroyImageView(textureImageView);
    device.destroyImageView(depthImageView);
    device.destroyImage(textureImage);
    device.destroyImage(depthImage);
    device.freeMemory(textureMemory);
    device.freeMemory(depthMemory);
    device.destroyDescriptorPool(descPool);
    device.destroyDescriptorSetLayout(uniformDesLayout);
    device.destroyBuffer(uniformBuffer);
    device.freeMemory(uniformMemory);
    device.freeMemory(indexMem);
    device.freeMemory(deviceMemory);
    device.freeMemory(vertexMemory);
    device.destroyBuffer(deviceBuffer);
    device.destroyBuffer(vertexBuffer);
    device.destroyBuffer(indexBuf);
    device.destroyFence(fence);
    device.destroySemaphore(imageAvaliableSem);
    device.destroySemaphore(renderFinishSem);
    device.freeCommandBuffers(cmdPool, cmdBuffer);
    device.destroyCommandPool(cmdPool);
    for(auto &frame: framebuffers){
        device.destroyFramebuffer(frame);
    }
    device.destroyRenderPass(renderPass);
    device.destroyPipelineLayout(layout);
    for(auto &pipe:pipelines) {
        device.destroyPipeline(pipe);
    }
    for(auto &module: modules) device.destroyShaderModule(module);
    for(auto &view: views){
        device.destroyImageView(view);
    }

    device.destroySwapchainKHR(swapChain);
    device.destroy();
    instance.destroySurfaceKHR(surface);    //在销毁instance之前销毁instance中的对象
    instance.destroy();
    //关闭窗口
    glfwDestroyWindow(window);
    glfwTerminate();
}

void TriangleApplication::loadTexture(){
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(texturePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    std::cout << "imageSize: " << imageSize << std::endl;
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    vk::Buffer tempBuf = createBuffer(vk::BufferUsageFlagBits::eTransferSrc,
                                vk::SharingMode::eExclusive,imageSize);
    vk::DeviceMemory tempMem = allocateMemory(tempBuf, vk::MemoryPropertyFlagBits::eHostVisible|
                                vk::MemoryPropertyFlagBits::eHostCoherent);
    device.bindBufferMemory(tempBuf, tempMem, 0);
    void* data = device.mapMemory(tempMem, 0, imageSize);
    memcpy(data, pixels, imageSize);
    device.unmapMemory(tempMem);

    stbi_image_free(pixels);

    createTextureImage(textureImage ,texHeight, texWidth ,vk::Format::eR8G8B8A8Unorm ,vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eTransferDst|vk::ImageUsageFlagBits::eSampled);

    transitionImageLayout(textureImage, vk::Format::eR8G8B8A8Unorm,
                vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
    copyBuffer2Image(tempBuf, textureImage, texHeight, texWidth);
    transitionImageLayout(textureImage, vk::Format::eR8G8B8A8Unorm,
                vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

    device.destroyBuffer(tempBuf);
    device.freeMemory(tempMem);        

}

vk::Image TriangleApplication::createImage(uint64_t height, uint64_t width, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage){
    vk::ImageCreateInfo info;
    vk::Extent3D extent;
    extent.setDepth(1)
          .setHeight(height)
          .setWidth(width);
    info.setImageType(vk::ImageType::e2D)
        .setMipLevels(1)
        .setFormat(format)
        .setExtent(extent)
        .setArrayLayers(1)
        .setTiling(tiling)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setSharingMode(vk::SharingMode::eExclusive)
        .setUsage(usage)
        .setSamples(vk::SampleCountFlagBits::e1);
    
    return device.createImage(info);
}

void TriangleApplication::createTextureImage(vk::Image image, uint64_t texHeight, uint64_t texWidth, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage){
    textureImage = createImage(texHeight, texWidth, format, tiling, usage);
    CHECK_NULL(textureImage);

    auto reqInfo = queryImageReqInfo(textureImage, vk::MemoryPropertyFlagBits::eDeviceLocal);
    vk::MemoryAllocateInfo allocateInfo;
    allocateInfo.setAllocationSize(reqInfo.size)
                .setMemoryTypeIndex(reqInfo.index);
    textureMemory = device.allocateMemory(allocateInfo);
    CHECK_NULL(textureMemory);
    device.bindImageMemory(textureImage, textureMemory, 0);
}

void TriangleApplication::transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout){
    vk::CommandBuffer cmdBuf = alloateCmdBuffer();
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

    if(newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal){
        barrier.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eDepth);
        if(hasStencilComponent(format)){
            barrier.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil);
        } else {
            barrier.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eDepth);
        }
    }
    if(oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal){
        barrier.setSrcAccessMask(vk::AccessFlagBits::eNone)
               .setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        dstStage = vk::PipelineStageFlagBits::eTransfer;
    } else if(oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal){
        barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
               .setDstAccessMask(vk::AccessFlagBits::eShaderRead);
        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        dstStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else if(oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal){
        barrier.setSrcAccessMask(vk::AccessFlagBits::eNone);
        barrier.setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentRead |
                                 vk::AccessFlagBits::eDepthStencilAttachmentWrite);

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        dstStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
    } else {
        throw std::runtime_error("Transfer ImageLayout Failed...");
    }

    cmdBuf.pipelineBarrier(sourceStage, dstStage, vk::DependencyFlagBits::eByRegion, 0, nullptr, 0, nullptr, 1, &barrier);
    cmdBuf.end();
    endCmdBuffer(cmdBuf);
}

void TriangleApplication::copyBuffer2Image(vk::Buffer buf, vk::Image image, uint32_t height, uint32_t width){
    vk::CommandBuffer cmdBuf = alloateCmdBuffer();

    vk::BufferImageCopy region;
    vk::ImageSubresourceLayers subSource;
    subSource.setBaseArrayLayer(0)
             .setMipLevel(0)
             .setAspectMask(vk::ImageAspectFlagBits::eColor)
             .setLayerCount(1);
    region.setBufferImageHeight(0)
          .setBufferOffset(0)
          .setBufferRowLength(0)
          .setImageOffset({0,0,0})
          .setImageExtent({width, height, 1})
          .setImageSubresource(subSource);

    cmdBuf.copyBufferToImage(buf, image, vk::ImageLayout::eTransferDstOptimal, 1, &region);
    cmdBuf.end();
    endCmdBuffer(cmdBuf);
}

vk::ImageView TriangleApplication::createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags){
     vk::ImageViewCreateInfo info;
    vk::ImageSubresourceRange range;
    range.setAspectMask(aspectFlags)
         .setBaseArrayLayer(0)
         .setBaseMipLevel(0)
         .setLayerCount(1)
         .setLevelCount(1);
    info.setImage(image)
        .setFormat(format)
        .setViewType(vk::ImageViewType::e2D)
        .setSubresourceRange(range);

    return device.createImageView(info);
}

void TriangleApplication::createTextureImageView(){
   textureImageView = createImageView(textureImage, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);
    CHECK_NULL(textureImageView);
}

void TriangleApplication::createTextureSample(){
    vk::SamplerCreateInfo info;
    info.setAddressModeU(vk::SamplerAddressMode::eRepeat)
        .setAddressModeV(vk::SamplerAddressMode::eRepeat)
        .setAddressModeW(vk::SamplerAddressMode::eRepeat)
        .setMagFilter(vk::Filter::eLinear)
        .setMinFilter(vk::Filter::eLinear)
        .setAnisotropyEnable(VK_FALSE)
        .setMaxAnisotropy(1)
        .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
        .setUnnormalizedCoordinates(VK_FALSE)
        .setCompareEnable(VK_FALSE)
        .setCompareOp(vk::CompareOp::eAlways)
        .setMipmapMode(vk::SamplerMipmapMode::eLinear)
        .setMipLodBias(0.0f)
        .setMaxLod(0.0f)
        .setMinLod(0.0f);
    textureSampler = device.createSampler(info);
    CHECK_NULL(textureSampler);
}

void TriangleApplication::createDepthResource(){
    vk::Format depthFormat = findDepthForamt();
    
    createDepthImage(supportInfo.extent.height, supportInfo.extent.width, depthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment);
    depthImageView = createImageView(depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth);

    transitionImageLayout(depthImage, depthFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);

}

vk::Format TriangleApplication::findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features){
    for(vk::Format format: candidates){
        vk::FormatProperties props;
        phyDevice.getFormatProperties(format, &props);
        if(tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features){
            return format;
        } else if(tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features){
            return format;
        } else {
            throw std::runtime_error("Find Supported Format Failed...");
        }
    }
    vk::Format fail;
    return fail;
}

vk::Format TriangleApplication::findDepthForamt(){
    return findSupportedFormat({vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eX8D24UnormPack32},
                         vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

bool TriangleApplication::hasStencilComponent(vk::Format format){
    return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eX8D24UnormPack32;
}

void TriangleApplication::createDepthImage(uint64_t depHeight, uint64_t depWidth, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage){
    depthImage = createImage(depHeight, depWidth, format, tiling, usage);
    CHECK_NULL(depthImage);

    auto reqInfo = queryImageReqInfo(depthImage, vk::MemoryPropertyFlagBits::eDeviceLocal);
    vk::MemoryAllocateInfo allocateInfo;
    allocateInfo.setAllocationSize(reqInfo.size)
                .setMemoryTypeIndex(reqInfo.index);
    depthMemory = device.allocateMemory(allocateInfo);
    CHECK_NULL(depthMemory);
    device.bindImageMemory(depthImage, depthMemory, 0);
}


