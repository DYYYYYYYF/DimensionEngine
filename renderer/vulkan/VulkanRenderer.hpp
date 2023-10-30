#pragma once

#include <iostream>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "../interface/IRenderer.hpp"
#include "../../application/Window.hpp"
#include "../../application/utils/EngineUtils.hpp"

/*
    Dont forget delete Init()
*/

namespace renderer {
    class VulkanRenderer : public IRenderer{
    public:
        VulkanRenderer();
        virtual ~VulkanRenderer();
        virtual bool Init() override;
        virtual void Release() override;
        virtual void CreateInstance() override;

    protected:
        vk::Instance _VkInstance;
    };// class VulkanRenderer
}// namespace renderer