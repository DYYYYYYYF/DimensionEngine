#pragma once
#include "interface/IRenderer.hpp"
#include "../vulkan/VkContext.hpp"

using namespace VkCore;

namespace renderer {
    class Renderer : public IRenderer{
    public:
        Renderer();
        virtual ~Renderer();
        virtual bool Init();
        virtual void Draw();

    protected:
        // Deprect
        VkContext* _Context;

    };
}


