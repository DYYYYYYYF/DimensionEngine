#pragma once
#include <_types/_uint32_t.h>
#include <vulkan/vulkan.hpp>
#include <optional>
#include "../../application/utils/EngineUtils.hpp"

namespace VkCore{

struct QueueFamilyProperty{
    std::optional<uint32_t> graphicsIndex;
    std::optional<uint32_t> presentIndex;
};

class IDevice{
public:
    IDevice(){}
    virtual ~IDevice(){}

    virtual vk::PhysicalDevice CreatePhysicalDeivce() = 0;
    virtual vk::Device CreateDevice() = 0;

protected:

protected:
    vk::PhysicalDevice _VkPhyDevice = nullptr;
    vk::Device _VkDevice = nullptr;
    QueueFamilyProperty _QueueFamileProp;

};
}
