#pragma once
#include <vulkan/vulkan.hpp>

namespace VkCore{
class Buffer{
public:
    Buffer();
    virtual ~Buffer();

public:



private:
    vk::Buffer _VkBuffer;


};
}
