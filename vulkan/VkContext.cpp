#include "VkContext.hpp"
#include "CmdBuffer.hpp"
#include "Device.hpp"
#include "Pipeline.hpp"
#include "RenderPass.hpp"
#include "SwapChain.hpp"
#include "interface/IDevice.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <fstream>
#include <vector>

using namespace udon;
using namespace VkCore;

VkContext::~VkContext(){
    Release();
}

VkContext::VkContext(){
    _Instance = nullptr;
    _Device = nullptr;
    _Surface = nullptr;
    _Swapchain = nullptr;
    _RenderPass = nullptr;
    _ICmdBuffer = nullptr;
}

bool VkContext::InitContext(){
    if (!CreateInstance()) return false;
    if (!CreatePhysicalDevice()) return false; 
    if (!CreateSurface()) return false;
    if (!CreateDevice()) return false;

    _QueueFamily = _Device->GetQueueFamily();
    if (!_Queue.InitQueue(_VkDevice, _QueueFamily)) return false;

    if (!CreateSwapchain()) return false;
    _Swapchain->GetImages(_Images);
    _Swapchain->GetImageViews(_Images, _ImageViews);

    if (!CreateRenderPass()) return false;
    if (!CreateCmdPool()) return false;
    if (!AllocateCmdBuffer()) return false;

    if (!CreateRenderPass()) return false;
    if (!CreateFrameBuffers()) return false;
    if (!InitSyncStructures()) return false;

    INFO("Init Pipeline");
    if (!InitPipeline()) return false;

    return true;
}

// Instance
bool VkContext::CreateInstance(){

    if (!InitWindow()){
        DEBUG("NULL WINDOW.");
        return false;
    }

    //Instance
    _Instance = new Instance();
    CHECK(_Instance);
    _VkInstance = _Instance->CreateInstance(_Window);
    CHECK(_VkInstance);

    return true;
}

bool VkContext::InitWindow(){
    _Window =  WsiWindow::GetInstance()->GetWindow();
    return true;
}

bool VkContext::CreatePhysicalDevice(){
    if (!_Device) _Device = new Device();
    CHECK(_Device);
    
    _VkPhyDevice = _Device->CreatePhysicalDeivce(_VkInstance);
    CHECK(_VkPhyDevice);

    return true;
}

bool VkContext::CreateDevice(){
    if (!_Device) _Device = new Device();
    CHECK(_Device);

    _VkDevice = _Device->CreateDevice(_SurfaceKHR);
    CHECK(_VkDevice);

    return true;
}

bool VkContext::CreateSurface(){
    _Surface = new Surface(_VkInstance, _Window);
    CHECK(_Surface);
    _SurfaceKHR = _Surface->CreateSurface();
    CHECK(_SurfaceKHR);

    return true;
}

bool VkContext::CreateSwapchain(){
    _Swapchain = new SwapChain(_SurfaceKHR, _QueueFamily);
    CHECK(_Swapchain);
    _Swapchain->SetDevice(_VkDevice);
    _SwapchainKHR = _Swapchain->CreateSwapchain(_VkPhyDevice);
    CHECK(_SwapchainKHR);

    return true;
}

bool VkContext::CreateRenderPass(){
    _RenderPass = new RenderPass();
    CHECK(_RenderPass);
    _RenderPass->SetDevice(_VkDevice);
    _RenderPass->SetPhyDevice(_VkPhyDevice);
    _RenderPass->SetSupportInfo(_Swapchain->GetSwapchainSupport());

    _VkRenderPass = _RenderPass->CreateRenderPass();
    CHECK(_VkRenderPass);
        
    return true;
}

bool VkContext::CreateCmdPool(){
    _ICmdBuffer = new CommandBuf();
    CHECK(_ICmdBuffer);
    if (!_CmdPool){
        _ICmdBuffer->CreateCmdPool(_Device);
    }

    _CmdPool = _ICmdBuffer->GetCmdPool();
    return true;
}

bool VkContext::AllocateCmdBuffer(){

    CHECK(_CmdPool);
    _CmdBuffer = _ICmdBuffer->AllocateCmdBuffer(_Device);
    CHECK(_CmdBuffer);

    return true;
}

bool VkContext::CreateFrameBuffers(){
 
    CHECK(_Swapchain);
    _Swapchain->CreateFrameBuffers(_Images, _ImageViews, _FrameBuffers, _RenderPass->GetRenderPass());

    for (int i=0; i<_FrameBuffers.size(); ++i){
        CHECK(_FrameBuffers[i]);
    }

    return true;
}

bool VkContext::InitSyncStructures(){
    vk::FenceCreateInfo FenceInfo;
    FenceInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
    _RenderFence = _VkDevice.createFence(FenceInfo);
    CHECK(_RenderFence);
 
    vk::SemaphoreCreateInfo SemapInfo;
    _RenderSemaphore = _VkDevice.createSemaphore(SemapInfo);
    CHECK(_RenderSemaphore);

    _PresentSemaphore = _VkDevice.createSemaphore(SemapInfo);
    CHECK(_PresentSemaphore);

    return true;
}

void VkContext::Draw(){
    if (_VkDevice.waitForFences(1, &_RenderFence, true, 1000000000) != vk::Result::eSuccess){
        return;
    }
    if (_VkDevice.resetFences(1, &_RenderFence) != vk::Result::eSuccess){
        return;
    }

    auto res = _VkDevice.acquireNextImageKHR(_SwapchainKHR, 1000000000, _PresentSemaphore, nullptr);
    uint32_t nSwapchainImageIndex = res.value;

    _CmdBuffer.reset();

    vk::CommandBufferBeginInfo info;
    info.setPInheritanceInfo(nullptr)
        .setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse);

    _CmdBuffer.begin(info);

    vk::ClearValue ClearValue;
    vk::ClearColorValue color = std::array<float, 4>{0.5f, 0.5f, 0.5f, 1.0f};
    ClearValue.setColor(color);

    vk::RenderPassBeginInfo rpInfo;
    vk::Extent2D extent = _Swapchain->GetSwapchainSupport().extent;
    rpInfo.setRenderPass(_RenderPass->GetRenderPass())
          .setRenderArea(vk::Rect2D({0, 0}, extent))
          .setFramebuffer(_FrameBuffers[nSwapchainImageIndex])
          .setClearValueCount(1)
          .setClearValues(ClearValue);
        
    _CmdBuffer.beginRenderPass(rpInfo, vk::SubpassContents::eInline);

    _CmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _Pipeline);
    _CmdBuffer.draw(3, 1, 0, 0);

    _CmdBuffer.endRenderPass();
    _CmdBuffer.end();

    vk::PipelineStageFlags WaitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submit;
    submit.setWaitSemaphoreCount(1)
        .setWaitSemaphores(_PresentSemaphore)
        .setSignalSemaphoreCount(1)
        .setSignalSemaphores(_RenderSemaphore)
        .setCommandBufferCount(1)
        .setCommandBuffers(_CmdBuffer)
        .setWaitDstStageMask(WaitStage);

    if (_Queue.GraphicsQueue.submit(1, &submit, _RenderFence) != vk::Result::eSuccess){
        return;
    }
    vk::PresentInfoKHR PresentInfo;
    PresentInfo.setSwapchainCount(1)
        .setSwapchains(_SwapchainKHR)
        .setWaitSemaphoreCount(1)
        .setWaitSemaphores(_RenderSemaphore)
        .setImageIndices(nSwapchainImageIndex);

    if (_Queue.GraphicsQueue.presentKHR(PresentInfo) != vk::Result::eSuccess){
        return;
    }

}

bool VkContext::CreateShaderModule(const char* shader_file, vk::ShaderModule& module){

    // Load file by binary
    std::ifstream file(shader_file, std::ios::binary | std::ios::in);
    std::vector<char> content((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
    file.close();

    vk::ShaderModuleCreateInfo info;
    info.setCodeSize(content.size())
        .setPCode((uint32_t*)content.data());

    module = _VkDevice.createShaderModule(info);
    CHECK(module);

    return true;
}

/*
*  Pipeline
*  All Stage
*/
vk::PipelineShaderStageCreateInfo VkContext::InitShaderStageCreateInfo(vk::ShaderStageFlagBits stage, vk::ShaderModule shader_module){
    vk::PipelineShaderStageCreateInfo info;
    info.setStage(stage)
            .setModule(shader_module)
            .setPName("main");

    return info;
}

vk::PipelineVertexInputStateCreateInfo VkContext::InitVertexInputStateCreateInfo(){
    vk::PipelineVertexInputStateCreateInfo info;
    info.setVertexBindingDescriptionCount(0)
        .setVertexAttributeDescriptionCount(0);

    return info;
}

vk::PipelineInputAssemblyStateCreateInfo VkContext::InitAssemblyStateCreateInfo(vk::PrimitiveTopology topology){
    vk::PipelineInputAssemblyStateCreateInfo info;
    info.setTopology(topology)
            .setPrimitiveRestartEnable(VK_FALSE);

    return info;
}

vk::PipelineRasterizationStateCreateInfo VkContext::InitRasterizationStateCreateInfo(vk::PolygonMode polygonMode){
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

vk::PipelineMultisampleStateCreateInfo VkContext::InitMultisampleStateCreateInfo(){
    vk::PipelineMultisampleStateCreateInfo info;
    info.setSampleShadingEnable(VK_FALSE)
        .setMinSampleShading(1.0f)
        .setAlphaToOneEnable(VK_FALSE)
        .setAlphaToCoverageEnable(VK_FALSE)
        .setPSampleMask(nullptr)
        .setRasterizationSamples(vk::SampleCountFlagBits::e1);

    return info;
}

vk::PipelineColorBlendAttachmentState VkContext::InitColorBlendAttachmentState(){
    vk::PipelineColorBlendAttachmentState info;
    info.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                           vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
    info.setBlendEnable(VK_FALSE);

    return info;
}

vk::PipelineLayoutCreateInfo VkContext::InitPipelineLayoutCreateInfo(){
    vk::PipelineLayoutCreateInfo info;
    info.setSetLayoutCount(0)
        .setPSetLayouts(nullptr)
        .setPushConstantRangeCount(0)
        .setPushConstantRanges(nullptr);

    return info;
}

bool VkContext::InitPipeline(){

    if (!CreateShaderModule("../shader/triangle_vert.spv", _VertShader)) return false;
    if (!CreateShaderModule("../shader/triangle_frag.spv", _FragShader)) return false;

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
    pipelineBuilder._Viewport.width = _Swapchain->GetSwapchainSupport().extent.width;
    pipelineBuilder._Viewport.height = _Swapchain->GetSwapchainSupport().extent.width;
    pipelineBuilder._Viewport.minDepth = 0.0f;
    pipelineBuilder._Viewport.maxDepth = 1.0f;

    INFO("Pipeline Scissor");
    pipelineBuilder._Scissor.offset = vk::Offset2D{0, 0};
    pipelineBuilder._Scissor.extent = _Swapchain->GetSwapchainSupport().extent;

    INFO("Pipeline Rasterizer");
    pipelineBuilder._Rasterizer = InitRasterizationStateCreateInfo(vk::PolygonMode::eFill);
    pipelineBuilder._Mutisampling = InitMultisampleStateCreateInfo();
    pipelineBuilder._ColorBlendAttachment = InitColorBlendAttachmentState();
    pipelineBuilder._PipelineLayout = _PipelineLayout;

    _Pipeline = pipelineBuilder.BuildPipeline(_VkDevice, _VkRenderPass);
    return true;
}

void VkContext::Release(){

}

