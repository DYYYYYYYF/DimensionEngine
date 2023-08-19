#include "CmdBuffer.hpp"
#include "Device.hpp"

using namespace VkCore;

CommandBuf::CommandBuf(){
    _VkCmdBuffer = nullptr;
    _VkCmdPool = nullptr;
}

CommandBuf::~CommandBuf(){}

vk::CommandPool CommandBuf::CreateCmdPool(Device* iDevice){
    CHECK(iDevice)

    vk::Device device = iDevice->GetDevice(); 
    CHECK(device);
    QueueFamilyProperty queueFamiylProp = iDevice->GetQueueFamily();

    vk::CommandPoolCreateInfo info;
    info.setQueueFamilyIndex(queueFamiylProp.graphicsIndex.value())
        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    _VkCmdPool = device.createCommandPool(info);

    CHECK(_VkCmdPool);
    return _VkCmdPool;
}

vk::CommandBuffer CommandBuf::AllocateCmdBuffer(Device* iDevice){
    CHECK(iDevice);

    if (!_VkCmdPool){
        _VkCmdPool = CreateCmdPool(iDevice);
    }

    vk::Device device = iDevice->GetDevice();
    CHECK(device);
    if (!device) return nullptr;

    vk::CommandBufferAllocateInfo allocte;
    allocte.setCommandPool(_VkCmdPool)
           .setCommandBufferCount(1)
           .setLevel(vk::CommandBufferLevel::ePrimary);
    
    _VkCmdBuffer = device.allocateCommandBuffers(allocte)[0];

    vk::CommandBufferBeginInfo info;
    info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    _VkCmdBuffer.begin(info);

    return _VkCmdBuffer;
}
