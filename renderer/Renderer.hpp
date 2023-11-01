#pragma once
#include "interface/IRenderer.hpp"
#include "interface/IRendererImpl.hpp"
#include "vulkan/VkMesh.hpp"

namespace renderer {
    class Renderer : public IRenderer{
    public:
        Renderer();
        virtual ~Renderer();
        virtual bool Init() override;
        virtual void CreatePipeline(Material& mat) override;
        virtual void Draw(RenderObject* first, int count) override;
        virtual void Release() override;

    public:
        void UploadMeshes(Mesh& mesh);
        void UpdateViewMat(glm::mat4 view_matrix);

    protected:

    };
}


