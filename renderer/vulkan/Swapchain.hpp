#pragma once
#include <vulkan/vulkan.hpp>

namespace renderer {
    struct SwapchainSupport {
        struct WindowSize{
            int width;
            int height;
        };

        WindowSize window_size;
        vk::SurfaceCapabilitiesKHR capabilities;      //能力
        vk::Extent2D extent;        //尺寸大小
        vk::SurfaceFormatKHR format;        //格式
        vk::PresentModeKHR presnetMode;     //显示模式
        uint32_t imageCount;

        void GetWindowSize() {
            udon::WsiWindow::GetInstance()->GetWindowSize(window_size.width, window_size.height);
        }

        bool QuerySupportInfo(const vk::PhysicalDevice& phyDevice, const vk::SurfaceKHR& surface) {

            CHECK(phyDevice);
            CHECK(surface);

            GetWindowSize();
            capabilities = phyDevice.getSurfaceCapabilitiesKHR(surface);
            auto surfaceFormats = phyDevice.getSurfaceFormatsKHR(surface);
            for (auto& surfaceFormat : surfaceFormats) {
                if (surfaceFormat.format == vk::Format::eR8G8B8A8Srgb ||
                    surfaceFormat.format == vk::Format::eB8G8R8A8Srgb ||
                    surfaceFormat.format == vk::Format::eR8G8B8A8Unorm) {
                    format = surfaceFormat;
                }
            }
            extent.width = std::clamp<uint32_t>(window_size.width,
                capabilities.minImageExtent.width,
                capabilities.maxImageExtent.width);
           extent.height = std::clamp<uint32_t>(window_size.height,
                capabilities.minImageExtent.height,
                capabilities.maxImageExtent.height);
            imageCount = std::clamp<uint32_t>(2,
                capabilities.minImageCount,
                capabilities.maxImageCount);
            presnetMode = vk::PresentModeKHR::eFifo;    //Default
            auto presentModes = phyDevice.getSurfacePresentModesKHR(surface);
            for (auto& mode : presentModes) {
                if (mode == vk::PresentModeKHR::eMailbox) {
                    presnetMode = mode;
                }
            }
            return true;
        }
    };
}



