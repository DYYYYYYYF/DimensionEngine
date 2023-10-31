#include "VkMesh.hpp"
#include "VkStructures.hpp"

using namespace renderer;

std::vector<vk::VertexInputBindingDescription> Vertex::GetBindingDescription() {
    static std::vector<vk::VertexInputBindingDescription> bingdings;
    vk::VertexInputBindingDescription binding;
    binding.setBinding(0)
        .setInputRate(vk::VertexInputRate::eVertex)
        .setStride(sizeof(Vertex));
    bingdings.push_back(binding);
    return bingdings;
}

std::array<vk::VertexInputAttributeDescription, 3> Vertex::GetAttributeDescription() {
    static std::array<vk::VertexInputAttributeDescription, 3> attributes;
    attributes[0].setBinding(0)
        .setLocation(0)      // .vert shader -- location
        .setFormat(vk::Format::eR32G32B32Sfloat)     //Same as.vert shader input type -- Vec2
        .setOffset(offsetof(Vertex, position));  //offset
    attributes[1].setBinding(0)
        .setLocation(1)
        .setFormat(vk::Format::eR32G32B32Sfloat)
        .setOffset(offsetof(Vertex, normal));
    attributes[2].setBinding(0)
        .setLocation(2)
        .setFormat(vk::Format::eR32G32B32Sfloat)
        .setOffset(offsetof(Vertex, color));
    /*attributes[3].setBinding(0)
        .setLocation(3)
        .setFormat(vk::Format::eR32G32Sfloat)
        .setOffset(offsetof(Vertex, texCoord));*/
    return attributes;
}