#pragma once

#include <vulkan/vulkan.hpp>
#include <optional>

namespace renderer{
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
}
