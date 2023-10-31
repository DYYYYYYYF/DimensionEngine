#pragma once

namespace renderer {
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
        virtual void CreatePipeline() = 0;

        virtual void DrawPerFrame() = 0;
    };
}// namespace renderer
