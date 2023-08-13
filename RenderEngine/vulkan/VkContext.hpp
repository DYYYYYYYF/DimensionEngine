#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>
#include "../engine/EngineLogger.hpp"
#include "../application/Window.hpp"

namespace VkCore{
class VkContext{
public:
    VkContext();
    virtual ~VkContext();

    bool InitInstance();

public:
    vk::Instance GetInstance() const {return _VkInstance;} 
    SDL_Window* GetWindow() const {return _Window;}

private:
    bool InitWindow();
    bool InitVulkan();
    void Release();

private:
    SDL_Window* _Window;
    vk::Instance _VkInstance;
    vk::PhysicalDevice _VkPhyDevice;
    vk::Device _VkDevice;

};
}


