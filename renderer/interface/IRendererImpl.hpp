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

    };
}// namespace renderer
