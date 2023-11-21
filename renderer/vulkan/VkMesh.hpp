#define TINYOBJLOADER_IMPLEMENTATION
#define GLM_ENABLE_EXPERIMENTAL
#pragma once

#include <iostream>
#include <string>
#include <glm/gtx/hash.hpp>

#include "VkStructures.hpp"

namespace renderer {

    struct Material {
        vk::DescriptorSet textureSet; 
        vk::Pipeline pipeline;
        vk::PipelineLayout pipelineLayout;
    };

    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 color;
        glm::vec2 texCoord;

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
    };

}

namespace std {
    template<> 
    struct hash<renderer::Vertex> {
        size_t operator()(const renderer::Vertex& vertex) const noexcept{
            return ((hash<glm::vec3>()(vertex.position) ^
                (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}
