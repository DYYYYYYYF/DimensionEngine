#pragma once
#include <vulkan/vulkan.hpp>

class IDevice{
public:
    IDevice(){}
    virtual ~IDevice(){}

    virtual vk::PhysicalDevice CreatePhysicalDeivce() = 0;
    virtual vk::Device CreateDevice() = 0;

protected:
    vk::PhysicalDevice _VkPhyDevice;
    vk::Device _VkDevice;

};
