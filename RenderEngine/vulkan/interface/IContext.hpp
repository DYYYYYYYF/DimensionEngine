#pragma once
#include <iostream>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "../../application/Window.hpp"
#include "../../application/utils/EngineUtils.hpp"
#include "vulkan/vulkan_handles.hpp"

namespace VkCore{
class IContext{
public:
    IContext(){}
    virtual ~IContext(){}

    virtual bool InitContext() = 0;
    virtual void Release() = 0;

protected:
    SDL_Window* _Window = nullptr;
    vk::Instance _VkInstance = nullptr;
    vk::PhysicalDevice _VkPhyDevice = nullptr;
    vk::Device _VkDevice = nullptr;
    vk::SurfaceKHR _SurfaceKHR = nullptr;
    vk::SwapchainKHR _SwapchainKHR = nullptr;
    vk::RenderPass _VkRenderPass = nullptr;
    vk::CommandPool _CmdPool = nullptr;
    vk::CommandBuffer _CmdBuffer = nullptr;
    vk::Semaphore _RenderSemaphore = nullptr;
    vk::Semaphore _PresentSemaphore = nullptr;
    vk::Fence _RenderFence = nullptr;

    std::vector<vk::Image> _Images;
    std::vector<vk::ImageView> _ImageViews;
    std::vector<vk::Framebuffer> _FrameBuffers;

    vk::ShaderModule _VertShader = nullptr;
    vk::ShaderModule _FragShader = nullptr;
    vk::PipelineLayout _PipelineLayout = nullptr;
    vk::Pipeline _Pipeline = nullptr;

};
}
