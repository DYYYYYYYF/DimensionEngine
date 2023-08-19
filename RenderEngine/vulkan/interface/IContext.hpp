#pragma once
#include <iostream>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "../../application/Window.hpp"
#include "../../application/utils/EngineUtils.hpp"

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
    std::vector<vk::Image> _Images;
    std::vector<vk::ImageView> _ImageViews;

};
}
