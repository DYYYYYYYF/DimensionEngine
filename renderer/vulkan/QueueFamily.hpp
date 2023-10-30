#pragma once
#include <vulkan/vulkan.hpp>

struct SwapchainSupport{
    vk::SurfaceCapabilitiesKHR capabilities;      //能力
    vk::Extent2D extent;        //尺寸大小
    vk::SurfaceFormatKHR format;        //格式
    vk::PresentModeKHR presnetMode;     //显示模式
    uint32_t imageCount;
};


