#pragma once

#include "../../engine/EngineStructures.hpp"

using namespace engine;

namespace renderer {
      class IRendererImpl;
      struct Mesh;
      struct Material;

	  class IRenderer {
	  public:
        IRenderer(){}
        virtual ~IRenderer() {}
        virtual bool Init() = 0;
        virtual void CreatePipeline(Material& mat, const char* vert_shader, const char* frag_shader, bool alpha = false) = 0;
        virtual void BeforeDraw() = 0;
        virtual void Draw(RenderObject* first, int count) = 0;
        virtual void AfterDraw() = 0;
        virtual void WaitIdel() = 0;
        virtual void Release() = 0;

    protected:
        IRendererImpl* _RendererImpl;

	};
}
