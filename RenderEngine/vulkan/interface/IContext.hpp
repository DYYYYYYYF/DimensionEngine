#pragma once
#include <iostream>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "../../engine/EngineLogger.hpp"
#include "../../application/Window.hpp"

namespace VkCore{
class IContext{
public:
    IContext(){}
    virtual ~IContext(){}

    virtual bool InitContext() = 0;
    virtual void Release() = 0;

protected:
    SDL_Window* _Window = nullptr;
    vk::Instance _VkInstance = nullptr;
    vk::PhysicalDevice _VkPhyDevice = nullptr;
    vk::Device _VkDevice = nullptr;

};
}
