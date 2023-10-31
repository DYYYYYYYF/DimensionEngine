#pragma once
#include "interface/IRenderer.hpp"

namespace renderer {
    class Renderer : public IRenderer{
    public:
        Renderer();
        virtual ~Renderer();
        virtual bool Init() override;
        virtual void Draw() override;

    protected:

    };
}


