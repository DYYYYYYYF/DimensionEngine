#pragma once

#include <vulkan/vulkan.hpp>
#include <optional>

namespace renderer{
    struct QueueFamilyProperty{
        std::optional<uint32_t> graphicsIndex = 0;
        std::optional<uint32_t> presentIndex = 0;
    };
}
