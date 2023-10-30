#pragma once

#ifdef DEBUG
#include <iostream>
#endif // DEBUG

#include <vector>
#include <vulkan/vulkan.hpp>
#include "../interface/IRendererImpl.hpp"
#include "../../application/Window.hpp"
#include "../../application/utils/EngineUtils.hpp"

#include "VkDevice.hpp"
/*
    Dont forget delete Init()
*/

namespace renderer {
    class VulkanRenderer : public IRendererImpl {
    public:
        VulkanRenderer();
        virtual ~VulkanRenderer();
        virtual bool Init() override;
        virtual void Release() override;
        virtual void InitWindow() override;
        virtual void CreateInstance() override;
        virtual void PickupPhyDevice() override;
        virtual void CreateSurface() override;
        virtual void CreateDevice() override;

    protected:
        bool QueryQueueFamilyProp();

    protected:
        QueueFamilyProperty _QueueFamilyProp;

        SDL_Window* _Window;
        vk::SurfaceKHR _Surface;
        vk::Instance _VkInstance;
        vk::PhysicalDevice _VkPhyDevice;
        vk::Device _VkDevice;

    };// class VulkanRenderer
}// namespace renderer
