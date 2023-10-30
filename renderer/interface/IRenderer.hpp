#pragma once
#include "../../vulkan/VkContext.hpp"

using namespace VkCore;

namespace renderer {
    class IRenderer {
    public:
        IRenderer() {}
        virtual ~IRenderer() {};
        virtual bool Init() = 0;
        virtual void Release() = 0;
        virtual void CreateInstance() = 0;

    };
}// namespace renderer
