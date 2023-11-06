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
        virtual void CreatePipeline(Material& mat, const char* vert_shader, const char* frag_shader) override;
        virtual void BeforeDraw() override;
        virtual void Draw(RenderObject* first, int count) override;
        virtual void AfterDraw() override;
        virtual void WaitIdel() override {_RendererImpl->WaitIdel();}
        virtual void Release() override;

    public:
        void UploadMeshes(Mesh& mesh);
        void UpdateViewMat(glm::mat4 view_matrix);
        void UnloadMeshes(std::unordered_map<std::string, Mesh>& meshes){_RendererImpl->UnloadMeshes(meshes);}
        void DestroyMaterials(std::unordered_map<std::string, Material>& materials){_RendererImpl->DestroyMaterials(materials);}

    protected:

    };
}


