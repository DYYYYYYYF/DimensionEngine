#pragma once
#include <vulkan/vulkan.hpp>
#include "SwapChain.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"

namespace VkCore{
class RenderPass{
public:
    RenderPass();
    virtual ~RenderPass();
    virtual vk::RenderPass CreateRenderPass();

public:
    void SetSupportInfo(const SwapchainSupport& supportInfo){ _SupportInfo = supportInfo; }
    void SetDevice(const vk::Device device) {_VkDevice = device;}
    void SetPhyDevice(const vk::PhysicalDevice phyDevice) {_VkPhyDevice = phyDevice;}

    vk::RenderPass GetRenderPass() const {return _VkRenderPass;}


private:
    vk::Format FindSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) const;
    vk::Format FindDepthFormat() const;


private:
    vk::Device _VkDevice;
    vk::PhysicalDevice _VkPhyDevice;
    vk::RenderPass _VkRenderPass;
    SwapchainSupport _SupportInfo;

};
}
