#pragma once
#include "interface/IDevice.hpp"
#include "vulkan/vulkan_handles.hpp"

namespace VkCore{

struct QueueFamilyIndex{

};

class Device : public IDevice{
public:
    Device();
    virtual ~Device();

    virtual vk::PhysicalDevice CreatePhysicalDeivce();
    virtual vk::PhysicalDevice CreatePhysicalDeivce(vk::Instance instance);
    virtual vk::Device CreateDevice();

private:
    void PickupPhysicsDevice(vk::Instance instance);

};
}
