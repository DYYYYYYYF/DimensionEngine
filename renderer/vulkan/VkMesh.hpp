#pragma once
#include "VkStructures.hpp"

namespace renderer {
    struct Vertex {
        Eigen::Vector3f position;
        Eigen::Vector3f normal;
        Eigen::Vector3f color;
    };

    struct Mesh {
        std::vector<Vertex> vertices;
        AllocatedBuffer vertexBuffer;
    };

}