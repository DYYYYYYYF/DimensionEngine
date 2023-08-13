#pragma once
#include "interface/IDevice.hpp"
#include "vulkan/vulkan_handles.hpp"

class Device : public IDevice{
public:
    Device();
    virtual ~Device();

    virtual vk::PhysicalDevice CreatePhysicalDeivce();
    virtual vk::Device CreateDevice();

private:
    void PickupPhysicsDevice(vk::Instance instance);

};
