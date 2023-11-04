#pragma once

#include <optional>
#include <iostream>
#include <vulkan/vulkan.hpp>
#include <unordered_map>
#include "../../engine/EngineLogger.hpp"
#include "../../application/Window.hpp"

#include <glm/glm.hpp>
namespace renderer{

    /*
        Vulkan Memory
    */
    struct MemRequiredInfo {
        uint32_t index;
        size_t size;
    };

    struct AllocatedBuffer {
        vk::Buffer buffer;  // CPU
        vk::DeviceMemory memory;    // GPU
    };

    struct AllocatedImage {
        vk::Image image;  // CPU
        vk::DeviceMemory memory;    // GPU
    };

    /*
        Texture
     */
    struct UploadContext{
        vk::Fence uploadFence;
        vk::CommandPool commandPool;
        vk::CommandBuffer commandBuffer;
    };

    struct Texture{
        AllocatedImage image;
        vk::ImageView imageView;
    };

    /*
        Vulkan Descriptor
    */
    // Dynamic buffer
    struct SceneData{
        glm::vec4 fogColor; // w is for exponent
        glm::vec4 fogDistances; // x for min, y for max

        glm::vec4 ambientColor; // w for sun power
        glm::vec4 sunlightDIrection;
        glm::vec4 sunlightColor;
    };

    // Uniform buffer
    struct CamerData {
        glm::mat4 view;
        glm::mat4 proj;
    };

    // Push constants
    struct MeshPushConstants {
        glm::vec4 data;
        glm::mat4 model;

    };

    /*
        Double Buffer
    */
    struct FrameData {
        vk::Semaphore presentSemaphore;
        vk::Semaphore renderSemaphore;
        vk::Fence renderFence;

        vk::CommandPool commandPool;
        vk::CommandBuffer mainCommandBuffer;

        // Buffer contains CameraData
        AllocatedBuffer cameraBuffer;
        vk::DescriptorSet globalDescriptor;
    };

    /*
        Vulkan Foundation
    */
    struct QueueFamilyProperty{
        std::optional<uint32_t> graphicsIndex = 0;
        std::optional<uint32_t> presentIndex = 0;
    };

    struct Queue {
        vk::Queue GraphicsQueue;
        vk::Queue PresentQueue;

        bool InitQueue(vk::Device device, const QueueFamilyProperty& queueFamily) {
            GraphicsQueue = device.getQueue(queueFamily.graphicsIndex.value(), 0);
            PresentQueue = device.getQueue(queueFamily.presentIndex.value(), 0);

            CHECK(GraphicsQueue);
            CHECK(PresentQueue);

            return GraphicsQueue && PresentQueue;
        }
    };

    struct SwapchainSupport {
        struct WindowSize {
            int width;
            int height;
        };

        WindowSize window_size;
        vk::SurfaceCapabilitiesKHR capabilities;      
        vk::Extent2D extent;        
        vk::SurfaceFormatKHR format;        
        vk::PresentModeKHR presnetMode;     
        uint32_t imageCount;

        int GetWindowWidth() const { return window_size.width; }
        int GetWindowHeight() const { return window_size.height; }

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
