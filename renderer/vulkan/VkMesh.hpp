#define TINYOBJLOADER_IMPLEMENTATION

#pragma once

#include <iostream>
#include <string>
#include "VkStructures.hpp"

namespace renderer {

    struct Material {
        vk::DescriptorSet textureSet{VK_NULL_HANDLE}; 
        vk::Pipeline pipeline;
        vk::PipelineLayout pipelineLayout;
    };

    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 color;
        glm::vec2 texCoord;

        static std::vector<vk::VertexInputBindingDescription> GetBindingDescription();
        static std::array<vk::VertexInputAttributeDescription, 4> GetAttributeDescription();

        bool operator==(const Vertex& other) const {
            return position == other.position && color == other.color /* && texCoord == other.texCoord*/;
        }
    };

    struct Mesh {
        std::vector<Vertex> vertices;
        AllocatedBuffer vertexBuffer;

        bool LoadFromObj(const char* filename);
    };

}
