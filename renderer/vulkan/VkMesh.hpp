#pragma once
#define TINYOBJLOADER_IMPLEMENTATION
#define GLM_ENABLE_EXPERIMENTAL

#include <iostream>
#include <string>
#include <glm/gtx/hash.hpp>

#include "VkStructures.hpp"

namespace renderer {

    struct ParticalData {
        Vector4 position;
        Vector4 velocity;
        Vector4 color;

        static vk::VertexInputBindingDescription GetBindingDescription();
        static std::array<vk::VertexInputAttributeDescription, 4> GetAttributeDescription();

        bool operator==(const ParticalData& other) const {
            return position == other.position && color == other.color && velocity == other.velocity;
        }
    };

    struct Material {
        vk::DescriptorSet textureSet; 
        vk::Pipeline pipeline;
        vk::PipelineLayout pipelineLayout;
    };

    struct Vertex {
        Vector3 position;
        Vector3 normal;
        Vector3 color;
        Vector2 texCoord;

        static vk::VertexInputBindingDescription GetBindingDescription();
        static std::array<vk::VertexInputAttributeDescription, 4> GetAttributeDescription();

        bool operator==(const Vertex& other) const {
            return position == other.position && color == other.color && texCoord == other.texCoord;
        }

    };

    struct Mesh {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        AllocatedBuffer vertexBuffer;
        AllocatedBuffer indexBuffer;

        bool LoadFromObj(const char* filename);

        void SetColor(float val) {
            for (auto& vert : vertices) {
                vert.color = { val, val, val };
            }
        }

        void SetColor(Vector3 color) {
            for (auto& vert : vertices) {
                vert.color = color;
            }
        }
    };

    struct Particals {
        std::vector<ParticalData> particals;
        std::vector<ParticalData> writeData;
        AllocatedBuffer readStorageBuffer;
        AllocatedBuffer writeStorageBuffer;

        Material* material = nullptr;

        void SetPartialCount(int size);
        size_t GetParticalCount() const { return particals.size(); }

        Material* GetMaterial() const { return material; }
        void SetMaterial(Material* mat) {
            if (mat == nullptr) {
                return;
            }

            material = mat;
        }
    };


}

namespace std {
    template<> 
    struct hash<renderer::Vertex> {
        size_t operator()(const renderer::Vertex& vertex) const noexcept{
            return ((hash<Vector3>()(vertex.position) ^
                (hash<Vector3>()(vertex.color) << 1)) >> 1) ^
                (hash<Vector2>()(vertex.texCoord) << 1);
        }
    };
}
