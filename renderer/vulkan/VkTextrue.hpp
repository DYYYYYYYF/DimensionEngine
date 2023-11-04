#pragma once
#include "VkStructures.hpp"
#include "VulkanRenderer.hpp"

namespace renderer{
    bool LoadImageFromFile(VulkanRenderer& renderer, const char* file, AllocatedImage& outImage);
}

