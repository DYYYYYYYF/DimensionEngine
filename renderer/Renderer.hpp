#pragma once
#include "interface/IRenderer.hpp"

class Renderer : public IRenderer{
public:
    Renderer();
    virtual ~Renderer();
    virtual bool Init();
    virtual void Draw();

private:

};


