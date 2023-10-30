#pragma once
#include "interface/IRenderer.hpp"

namespace renderer {
    class Renderer{
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


