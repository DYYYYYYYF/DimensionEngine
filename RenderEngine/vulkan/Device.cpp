#include "Device.hpp"
#include <iostream>

using namespace VkCore; 

Device::Device(){

}

Device::~Device(){

}

vk::PhysicalDevice Device::CreatePhysicalDeivce(){
    return nullptr;
}

void Device::PickupPhysicsDevice(vk::Instance instance){
    std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
    std::cout << "\nUsing GPU:  " << physicalDevices[0].getProperties().deviceName << std::endl;    //输出显卡名称
    _VkPhyDevice = physicalDevices[0];
}

vk::PhysicalDevice Device::CreatePhysicalDeivce(vk::Instance instance){
    PickupPhysicsDevice(instance);
    CHECK(_VkPhyDevice);

    return _VkPhyDevice;
}

vk::Device Device::CreateDevice(){

    return nullptr;
}
