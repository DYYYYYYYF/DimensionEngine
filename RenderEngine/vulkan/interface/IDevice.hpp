#pragma once
#include <vulkan/vulkan.hpp>
#include "../../application/utils/EngineUtils.hpp"

namespace VkCore{
class IDevice{
public:
    IDevice(){}
    virtual ~IDevice(){}

    virtual vk::PhysicalDevice CreatePhysicalDeivce() = 0;
    virtual vk::Device CreateDevice() = 0;

protected:
    vk::PhysicalDevice _VkPhyDevice = nullptr;
    vk::Device _VkDevice = nullptr;

};
}
