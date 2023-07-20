#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

namespace VkCore{
class VkContext{
public:
    virtual bool InitWindow();
    virtual bool InitVulkan();

private:
    vk::Instance* _VkInstance;


};
}


