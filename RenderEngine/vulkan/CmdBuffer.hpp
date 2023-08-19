#pragma once
#include <vulkan/vulkan.hpp>

namespace VkCore{

class Device;

class CommandBuf{
public:
    CommandBuf();
    virtual ~CommandBuf();

    vk::CommandPool CreateCmdPool(Device* iDevice);
    vk::CommandBuffer AllocateCmdBuffer(Device* iDevice);

private:
    vk::CommandPool _VkCmdPool;
    vk::CommandBuffer _VkCmdBuffer;

};
}
