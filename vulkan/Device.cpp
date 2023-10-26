#include "Device.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
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

vk::Device Device::CreateDevice(vk::SurfaceKHR surface){
    if (!QueryQueueFamilyProp(surface)){
        INFO("Query Queue Family properties failed.");
        return nullptr;
    }

    std::vector<vk::DeviceQueueCreateInfo> dqInfo;
    //两者索引值相同，只需要创建一个实体
    if(_QueueFamileProp.graphicsIndex.value() == _QueueFamileProp.presentIndex.value()){
        vk::DeviceQueueCreateInfo info;
        float priority = 1.0;
        info.setQueuePriorities(priority);      //优先级
        info.setQueueCount(1);          //队列个数
        info.setQueueFamilyIndex(_QueueFamileProp.graphicsIndex.value());     //下标  
        dqInfo.push_back(info);        
    } else {
        vk::DeviceQueueCreateInfo info1;
        float priority = 1.0;
        info1.setQueuePriorities(priority); 
        info1.setQueueCount(1);
        info1.setQueueFamilyIndex(_QueueFamileProp.graphicsIndex.value());

        vk::DeviceQueueCreateInfo info2;
        info2.setQueuePriorities(priority);
        info2.setQueueCount(1);
        info2.setQueueFamilyIndex(_QueueFamileProp.presentIndex.value());

        dqInfo.push_back(info1);       
        dqInfo.push_back(info2);        
    }

    vk::DeviceCreateInfo dInfo;
    dInfo.setQueueCreateInfos(dqInfo);
    std::array<const char*,2> extensions{"VK_KHR_portability_subset",
                                          VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    dInfo.setPEnabledExtensionNames(extensions);

    #ifdef __BUILD__
        std::cout << _QueueFamileProp.graphicsIndex.value() << " " << _QueueFamileProp.presentIndex.value() << std::endl;
    #endif

    _VkDevice = _VkPhyDevice.createDevice(dInfo);
    CHECK(_VkDevice);
    return _VkDevice;
}

bool Device::QueryQueueFamilyProp(vk::SurfaceKHR surface){
    CHECK(_VkPhyDevice)
    CHECK(surface);
     
    auto families = _VkPhyDevice.getQueueFamilyProperties();
    uint32_t index = 0;
    for(auto &family:families){
        //选取Graph相关指令
        if(family.queueFlags | vk::QueueFlagBits::eGraphics){
            _QueueFamileProp.graphicsIndex = index;
        }
        //选取Surface相关指令
        if(_VkPhyDevice.getSurfaceSupportKHR(index, surface)){
            _QueueFamileProp.presentIndex = index;
        }
        //如果都找到对应序列索引，直接退出
        if(_QueueFamileProp.graphicsIndex && _QueueFamileProp.presentIndex) {
            break;
        }
        index++;
    }

    return _QueueFamileProp.graphicsIndex && _QueueFamileProp.presentIndex;
}
