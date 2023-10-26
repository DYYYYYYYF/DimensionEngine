#pragma once
#include "interface/IContext.hpp"
#include "Instance.hpp"
#include "Device.hpp"
#include "Surface.hpp"
#include "Queue.hpp"
#include "SwapChain.hpp"
#include "RenderPass.hpp"
#include "CmdBuffer.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"

namespace VkCore{
class VkContext : public IContext{
public:
    VkContext();
    virtual ~VkContext();
    virtual bool InitContext();
    virtual void Release();

public:
    bool CreateInstance();
    bool CreatePhysicalDevice();
    bool CreateDevice();
    bool CreateSurface();
    bool CreateSwapchain();
    bool CreateRenderPass();
    bool CreateCmdPool();
    bool AllocateCmdBuffer();
    bool CreateFrameBuffers();
    bool InitSyncStructures();
    bool CreateShaderModule(const char* shader_file, vk::ShaderModule& module);
    bool InitPipeline();

    // Pipeline
    vk::PipelineShaderStageCreateInfo InitShaderStageCreateInfo(vk::ShaderStageFlagBits stage, vk::ShaderModule shader_module);
    vk::PipelineVertexInputStateCreateInfo InitVertexInputStateCreateInfo();
    vk::PipelineInputAssemblyStateCreateInfo InitAssemblyStateCreateInfo(vk::PrimitiveTopology topology);
    vk::PipelineRasterizationStateCreateInfo InitRasterizationStateCreateInfo(vk::PolygonMode polygonMode);
    vk::PipelineMultisampleStateCreateInfo InitMultisampleStateCreateInfo();
    vk::PipelineColorBlendAttachmentState InitColorBlendAttachmentState();
    vk::PipelineLayoutCreateInfo InitPipelineLayoutCreateInfo();

    void Draw();

    SDL_Window* GetWindow() const {return _Window;}
    vk::Instance GetInstance() const {return _VkInstance;} 
    vk::Device GetDevice() const {return _VkDevice;}
    vk::SurfaceKHR GetSurface() const {return _SurfaceKHR;}
    vk::SwapchainKHR GetSwapchain() const {return _SwapchainKHR;}
    vk::RenderPass GetRenderPass() const {return _VkRenderPass;}
    vk::CommandPool GetCmdPool() const {return _CmdPool;}
    vk::CommandBuffer GetCommandBuf() const {return _CmdBuffer;}

private:
    bool InitWindow();

private:
    Instance* _Instance;
    Device* _Device;
    Surface* _Surface;
    QueueFamilyProperty _QueueFamily;
    Queue _Queue;
    SwapChain* _Swapchain;
    RenderPass* _RenderPass;
    CommandBuf* _ICmdBuffer;
};
}


