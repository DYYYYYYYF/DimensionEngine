#define TINYOBJLOADER_IMPLEMENTATION

#pragma once

#include <iostream>
#include <string>
#include "VkStructures.hpp"

namespace renderer {

    struct Vertex {
        Eigen::Vector3f position;
        Eigen::Vector3f normal;
        Eigen::Vector3f color;

        static std::vector<vk::VertexInputBindingDescription> GetBindingDescription();
        static std::array<vk::VertexInputAttributeDescription, 3> GetAttributeDescription();

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
