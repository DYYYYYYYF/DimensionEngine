#pragma once
#include "interface/IRenderer.hpp"
#include "vulkan/VulkanRenderer.hpp"
#include "vulkan/VkMesh.hpp"
#include "vulkan/VkTextrue.hpp"
#include "../engine/resource/ConfigFile.hpp"
#include <filesystem>

namespace renderer {
    class Renderer : public IRenderer{
    public:
        Renderer();
        virtual ~Renderer();
        virtual bool Init() override;
        virtual void CreatePipeline(Material& mat, const char* vert_shader, const char* frag_shader, bool alpha = false) override;
        virtual void BeforeDraw() override;
        virtual void Draw(RenderObject* first, size_t count) override;
        virtual void AfterDraw() override;
        virtual void WaitIdel() override {_RendererImpl->WaitIdel();}
        virtual void Release() override;

        // Draw common
        virtual void DrawPoint(Vector3 position, Vector3 color) override;
        virtual void DrawLine(Vector3 p1, Vector3 p2, Vector3 color) override;
        virtual void DrawRectangle(Vector3 position, Vector3 half_extent, Vector3 color, bool is_fill = true) override;
        virtual void DrawCircle(Vector3 position, float radius, Vector3 color, bool is_fill = true, int side_count = 365) override;

    public:
        void UpdateViewMat(Matrix4 view_matrix, Vector3 view_pos);

        void LoadTexture(const char* filename, const char* texture_path);
        void CreateDrawlinePipeline(Material& mat, const char* vert_shader, const char* frag_shader);
        void CreateComputePipeline(Material& mat, const char* comp_shader);

        void LoadMesh(const char* filename, Mesh& mesh);
        void LoadMesh(const char* filename, const char* mesh_name);
        void LoadPartical(Particals partical);
        void LoadTriangleMesh();

        void CreateMaterial(const char* filename, const char* vertShader, const char* fragShader);
        Material* GetMaterial(const std::string& name);
        Mesh* GetMesh(const std::string& name);
        Texture* GetTexture(const std::string& name);

        void ReleaseMeshes() { ((VulkanRenderer*)_RendererImpl)->ReleaseMeshes(_Meshes); }
        void ReleaseMaterials() { ((VulkanRenderer*)_RendererImpl)->ReleaseMaterials(_Materials); }
        void ReleaseTextures() { ((VulkanRenderer*)_RendererImpl)->ReleaseTextures( _Textures); }
        void ReleaseParticals() { ((VulkanRenderer*)_RendererImpl)->ReleaseBuffer(_Particals); }

        void SetEnableTexture(bool val) {
            ((VulkanRenderer*)_RendererImpl)->SetEnabledTexture(val);
        }
        void BindTexture(Material* material, const char* texture_name)
        {
            ((VulkanRenderer*)_RendererImpl)->BindTextureDescriptor(material, GetTexture(texture_name));
        }

        void SetConfigFile(ConfigFile* config) { 
            if (config == nullptr) {
                return;
            }

            _ConfigFile = config;
            std::string strPrePath = _ConfigFile->GetVal("PrePath");
            if (strPrePath.size() == 0) {
                return;
            }

            std::filesystem::path cur_path = std::filesystem::current_path();
            _RootDirection = cur_path.string() + "/" + strPrePath;
        }

        void SetRenderObjects(std::vector<RenderObject>* objs) {
            _Renderables = objs;
        }

    private:
        void AddMesh(std::string name, Mesh mesh) { _Meshes[name] = mesh; }
        void AddMaterial(std::string name, Material mat) { _Materials[name] = mat; }
        void AddTexture(std::string name, Texture tex) { _Textures[name] = tex; }

        std::string _RootDirection = "../";

    protected:
        std::vector<Particals> _Particals;
        std::vector<RenderObject>* _Renderables;
        
        std::unordered_map<std::string, Mesh> _Meshes;
        std::unordered_map<std::string, Material> _Materials;
        std::unordered_map<std::string, Texture> _Textures;

        ConfigFile* _ConfigFile;
    };
}


