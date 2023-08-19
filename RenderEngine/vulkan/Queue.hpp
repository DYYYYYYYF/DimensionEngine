#pragma once
#include <vulkan/vulkan.hpp>
#include "Device.hpp"
// #include "interface/IDevice.hpp"

namespace VkCore{
struct Queue{
    vk::Queue GraphicsQueue;
    vk::Queue PresentQueue;

    bool InitQueue(vk::Device device, QueueFamilyProperty queueFamily){
        GraphicsQueue = device.getQueue(queueFamily.graphicsIndex.value(), 0);
        PresentQueue = device.getQueue(queueFamily.presentIndex.value(), 0);

        return GraphicsQueue && PresentQueue;
    }
};
}
