#pragma once

#ifdef DEBUG
#include <iostream>
#endif // DEBUG

#include <vector>
#include <vulkan/vulkan.hpp>
#include "../interface/IRendererImpl.hpp"
#include "../../application/Window.hpp"
#include "../../application/utils/EngineUtils.hpp"

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
        virtual void CreateInstance() override;
        virtual void CreatePhyDevice() override;

    protected:
        vk::Instance _VkInstance;
        vk::PhysicalDevice _VkPhyDevice;

    };// class VulkanRenderer
}// namespace renderer