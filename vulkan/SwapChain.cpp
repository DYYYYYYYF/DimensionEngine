#include "SwapChain.hpp"
#include "vulkan/vulkan_handles.hpp"

using namespace VkCore;
SwapChain::SwapChain(vk::SurfaceKHR surface, VkCore::QueueFamilyProperty queueFamily){
    w = 0; h = 0;
    _SurfaceKHR = surface;
    CHECK(_SurfaceKHR);

    _QueueFamily = queueFamily;
}

SwapChain::~SwapChain(){}

void SwapChain::GetWindowSize(){
    udon::WsiWindow::GetInstance()->GetWindowSize(w, h);
    std::cout << w << " " << h << "\n";
}

bool SwapChain::QuerySupportInfo(vk::PhysicalDevice phyDevice){

    CHECK(phyDevice);
    CHECK(_SurfaceKHR);

    GetWindowSize();
    _SupportInfo.capabilities = phyDevice.getSurfaceCapabilitiesKHR(_SurfaceKHR);
    auto formats = phyDevice.getSurfaceFormatsKHR(_SurfaceKHR);
    for(auto &format: formats){
        if(format.format == vk::Format::eR8G8B8A8Srgb || 
           format.format == vk::Format::eB8G8R8A8Srgb || 
           format.format == vk::Format::eR8G8B8A8Unorm){
            _SupportInfo.format = format;
        }
    }
    _SupportInfo.extent.width = std::clamp<uint32_t>(w, 
                            _SupportInfo.capabilities.minImageExtent.width,
                            _SupportInfo.capabilities.maxImageExtent.width);
    _SupportInfo.extent.height = std::clamp<uint32_t>(h, 
                            _SupportInfo.capabilities.minImageExtent.height,
                            _SupportInfo.capabilities.maxImageExtent.height);
    _SupportInfo.imageCount = std::clamp<uint32_t>(2,
                            _SupportInfo.capabilities.minImageCount,
                            _SupportInfo.capabilities.maxImageCount);
    _SupportInfo.presnetMode = vk::PresentModeKHR::eFifo;    //默认值--先进先出绘制图像
    auto presentModes = phyDevice.getSurfacePresentModesKHR(_SurfaceKHR);
    for(auto& mode:presentModes){
        if(mode == vk::PresentModeKHR::eMailbox){
            _SupportInfo.presnetMode = mode;
        }
    }

    return true;
}

vk::SwapchainKHR SwapChain::CreateSwapchain(vk::PhysicalDevice phyDevice){
    QuerySupportInfo(phyDevice);

    vk::SwapchainCreateInfoKHR scInfo;
    scInfo.setImageColorSpace(_SupportInfo.format.colorSpace);
    scInfo.setImageFormat(_SupportInfo.format.format);
    scInfo.setImageExtent(_SupportInfo.extent);
    scInfo.setMinImageCount(_SupportInfo.imageCount);
    scInfo.setPresentMode(_SupportInfo.presnetMode);
    scInfo.setPreTransform(_SupportInfo.capabilities.currentTransform);
    
    if(_QueueFamily.graphicsIndex.value() == _QueueFamily.presentIndex.value()){
        scInfo.setQueueFamilyIndices(_QueueFamily.graphicsIndex.value());
        scInfo.setImageSharingMode(vk::SharingMode::eExclusive);
    } else {
        std::array<uint32_t,2> index{_QueueFamily.graphicsIndex.value(),
                                     _QueueFamily.presentIndex.value()};
        scInfo.setQueueFamilyIndices(index);
        scInfo.setImageSharingMode(vk::SharingMode::eConcurrent);
    }

    scInfo.setClipped(true);
    scInfo.setSurface(_SurfaceKHR);
    scInfo.setImageArrayLayers(1);
    scInfo.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
    scInfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);

    CHECK(_VkDevice);
    _SwapchainKHR = _VkDevice.createSwapchainKHR(scInfo);

    CHECK(_SwapchainKHR);
    return _SwapchainKHR;
}

void SwapChain::GetImageViews(const std::vector<vk::Image>& images, std::vector<vk::ImageView>& views) const {
    views.resize(images.size());
    for(int i=0; i<images.size(); i++){
        vk::ImageViewCreateInfo info;
        info.setImage(images[i]);     //纹理贴图
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
        views[i] = _VkDevice.createImageView(info);
    }
}

void SwapChain::CreateFrameBuffers(const std::vector<vk::Image>& images, const std::vector<vk::ImageView>& views,
                                   std::vector<vk::Framebuffer>& frameBuffers, const vk::RenderPass& renderpass){
       
    const uint32_t nSwapchainCount = images.size();
    frameBuffers = std::vector<vk::Framebuffer>(nSwapchainCount);

    for (int i=0; i<nSwapchainCount; ++i){
        vk::FramebufferCreateInfo info;
        info.setRenderPass(renderpass)
            .setAttachmentCount(1)
            .setWidth(w)
            .setHeight(h)
            .setLayers(1)
            .setPAttachments(&views[i]);
        frameBuffers[i] = _VkDevice.createFramebuffer(info);
    }
}
