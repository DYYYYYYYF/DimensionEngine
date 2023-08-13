#include "Device.hpp"
#include "vulkan/vulkan_handles.hpp"
#include <iostream>

Device::Device(){

}

Device::~Device(){

}

void Device::PickupPhysicsDevice(vk::Instance instance){
    std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
    std::cout << "Using GPU:" << physicalDevices[0].getProperties().deviceName << std::endl;    //输出显卡名称
    _VkPhyDevice = physicalDevices[0];
}

vk::PhysicalDevice Device::CreatePhysicalDeivce(){

    return nullptr;
}

vk::Device Device::CreateDevice(){

    return nullptr;
}
