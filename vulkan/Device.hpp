#pragma once
#include <iostream>
#include "interface/IDevice.hpp"

namespace VkCore{
class Device : public IDevice{
public:
    Device();
    virtual ~Device();

    virtual vk::PhysicalDevice CreatePhysicalDeivce();
    virtual vk::PhysicalDevice CreatePhysicalDeivce(vk::Instance instance);
    virtual vk::Device CreateDevice();
    virtual vk::Device CreateDevice(vk::SurfaceKHR surface);

public:
    QueueFamilyProperty GetQueueFamily() const {return _QueueFamileProp;}
    vk::Device GetDevice() const {return _VkDevice;}
    vk::PhysicalDevice GetPhyDevice() const {return _VkPhyDevice;}

protected:
    bool QueryQueueFamilyProp(vk::SurfaceKHR surface);
    void PickupPhysicsDevice(vk::Instance instance);


};
}
