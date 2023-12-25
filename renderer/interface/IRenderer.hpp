#pragma once

#include "../../engine/EngineStructures.hpp"

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
        virtual void Draw(RenderObject* first, size_t count) = 0;
        virtual void AfterDraw() = 0;
        virtual void WaitIdel() = 0;
        virtual void Release() = 0;

        virtual void DrawPoint(Vector3 position, Vector3 color) = 0;
        virtual void DrawLine(Vector3 p1, Vector3 p2, Vector3 color) = 0;
        virtual void DrawRectangle(Vector3 position, Vector3 half_extent, Vector3 color, bool is_fill = true) = 0;
        virtual void DrawCircle(Vector3 position, float radius, Vector3 color, bool is_fill = true, int side_count = 365) = 0;

    protected:
        IRendererImpl* _RendererImpl;

	};
}
