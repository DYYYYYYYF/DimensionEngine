#pragma once
#include "interface/IRenderer.hpp"
#include "vulkan/VulkanRenderer.hpp"
#include "vulkan/VkMesh.hpp"
#include "vulkan/VkTextrue.hpp"
#include "../engine/resource/ConfigFile.hpp"

namespace renderer {
    class Renderer : public IRenderer{
    public:
        Renderer();
        virtual ~Renderer();
        virtual bool Init() override;
        virtual void CreatePipeline(Material& mat, const char* vert_shader, const char* frag_shader, bool alpha = false) override;
        virtual void BeforeDraw() override;
        virtual void Draw(RenderObject* first, int count) override;
        virtual void AfterDraw() override;
        virtual void WaitIdel() override {_RendererImpl->WaitIdel();}
        virtual void Release() override;

    public:
        void UpdateViewMat(glm::mat4 view_matrix);

        void LoadTexture(const char* filename, const char* texture_path);
        void CreateDrawlinePipeline(Material& mat, const char* vert_shader, const char* frag_shader){
            ((VulkanRenderer*)_RendererImpl)->CreateDrawLinePipeline(mat, vert_shader, frag_shader);
        }

        void LoadMesh(const char* filename, Mesh& mesh);
        void LoadMesh(const char* filename, const char* mesh_name);
        void LoadTriangleMesh();
        void LoadRectangleMesh();

        Material* GetMaterial(const std::string& name);
        Mesh* GetMesh(const std::string& name);
        Texture* GetTexture(const std::string& name);

        void ReleaseMeshes() { ((VulkanRenderer*)_RendererImpl)->ReleaseMeshes(_Meshes); }
        void ReleaseMaterials() { ((VulkanRenderer*)_RendererImpl)->ReleaseMaterials(_Materials); }
        void ReleaseTextures() { ((VulkanRenderer*)_RendererImpl) -> ReleaseTextures( _Textures); }

        void SetConfigFile(ConfigFile* config) { _ConfigFile = config; }

    private:
        void AddMesh(std::string name, Mesh mesh) { _Meshes[name] = mesh; }
        void AddMaterial(std::string name, Material mat) { _Materials[name] = mat; }
        void AddTexture(std::string name, Texture tex) { _Textures[name] = tex; }

    protected:
        std::unordered_map<std::string, Mesh> _Meshes;
        std::unordered_map<std::string, Material> _Materials;
        std::unordered_map<std::string, Texture> _Textures;

        ConfigFile* _ConfigFile;
    };
}


