#pragma once

namespace renderer {
    class IRendererImpl {
    public:
        IRendererImpl() {}
        virtual ~IRendererImpl() {};
        virtual bool Init() = 0;
        virtual void Release() = 0;
        virtual void CreateInstance() = 0;
        virtual void CreatePhyDevice() = 0;

    };
}// namespace renderer
