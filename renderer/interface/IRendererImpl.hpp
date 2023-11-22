#pragma once

#include "../../engine/EngineStructures.hpp"

namespace renderer {
    struct Mesh;
    struct Material;

    class IRendererImpl {
    public:
        IRendererImpl() {}
        virtual ~IRendererImpl() {};
        virtual bool Init() = 0;
        virtual void Release() = 0;
        virtual void InitWindow() = 0;
        virtual void CreateInstance() = 0;
        virtual void PickupPhyDevice() = 0;
        virtual void CreateSurface() = 0;
        virtual void CreateDevice() = 0;
        virtual void CreateSwapchain() = 0;
        virtual void CreateRenderPass() = 0;
        virtual void CreateCmdPool() = 0;
        virtual void CreateFrameBuffers() = 0;
        virtual void InitSyncStructures() = 0;
        virtual void CreatePipeline(Material& mat, const char* vert_shader, const char* frag_shader) = 0;

        virtual void DrawPerFrame(RenderObject* first, int count) = 0;

        virtual void UpLoadMeshes(Mesh& mesh) = 0;

        virtual void WaitIdel() = 0;
    };
}// namespace renderer
