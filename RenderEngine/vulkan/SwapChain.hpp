#pragma once
#include <vulkan/vulkan.hpp>
#include "Device.hpp"
#include "interface/IDevice.hpp"
#include <../../application/utils/EngineUtils.hpp>
#include <../../application/Window.hpp>

namespace VkCore{

struct SwapchainSupport{
    vk::SurfaceCapabilitiesKHR capabilities;      //能力
    vk::Extent2D extent;        //尺寸大小
    vk::SurfaceFormatKHR format;        //格式
    vk::PresentModeKHR presnetMode;     //显示模式
    uint32_t imageCount;
};

class SwapChain{
public:
    SwapChain(){}
    SwapChain(vk::SurfaceKHR surface, QueueFamilyProperty queueFamily);
    virtual ~SwapChain();
    
    virtual vk::SwapchainKHR CreateSwapchain(vk::PhysicalDevice phyDevice);

public:
    void SetDevice(vk::Device device){_VkDevice = device;}
    void GetImages(std::vector<vk::Image>& images) const {
        images = _VkDevice.getSwapchainImagesKHR(_SwapchainKHR);
    }
    void GetImageViews(const std::vector<vk::Image>& images, std::vector<vk::ImageView>& views) const;
    SwapchainSupport GetSwapchainSupport() const {return _SupportInfo;}

private:
    bool QuerySupportInfo(vk::PhysicalDevice phyDevice);
    void GetWindowSize();

private:
    int w, h;
    vk::SurfaceKHR _SurfaceKHR;
    vk::Device _VkDevice;
    vk::SwapchainKHR _SwapchainKHR;

    SwapchainSupport _SupportInfo;
    QueueFamilyProperty _QueueFamily;

};
}
