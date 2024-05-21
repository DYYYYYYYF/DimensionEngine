#pragma once

#include "Defines.hpp"
#include <vector>

struct SPlatformState;
class VulkanContext;

bool PlatformCreateVulkanSurface(
	SPlatformState* plat_state,
	VulkanContext* context
);

void GetPlatformRequiredExtensionNames(std::vector<const char*>& array);