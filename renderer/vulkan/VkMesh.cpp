#include "VkMesh.hpp"
#include "VkStructures.hpp"
#include <tiny_obj_loader.h>
#include <unordered_map>

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

std::array<vk::VertexInputAttributeDescription, 4> Vertex::GetAttributeDescription() {
    static std::array<vk::VertexInputAttributeDescription, 4> attributes;
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
    attributes[3].setBinding(0)
        .setLocation(3)
        .setFormat(vk::Format::eR32G32Sfloat)
        .setOffset(offsetof(Vertex, texCoord));
    return attributes;
}

bool Mesh::LoadFromObj(const char* filename){
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    std::unordered_map<Vertex, uint32_t> uniqueVertices;

    if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename)){
        WARN(("Load %s failed", filename));
        return false;
    }

	INFO("\n Loading %s :	\n\
	\t Shape count : %d		\n	\t Vertex count: %d		\n\
	\t Normal count: %d		\n	\t UV count: %d			\n\
	\t Sub Model count: %d	\n	\t Material count: %d",
	filename, shapes.size(), attrib.vertices.size() / 3, attrib.normals.size() / 3, attrib.texcoords.size() / 2, shapes.size(), materials.size());

	vertices.clear();

    // Loop over shapes
    for (size_t i = 0; i < shapes.size(); ++i) {

		// Loop over faces(polygon)
        for (const auto& index : shapes[i].mesh.indices) {

            Vertex new_vert = {};

            //vertex position
            new_vert.position[0] = attrib.vertices[3 * index.vertex_index + 0];
            new_vert.position[1] = attrib.vertices[3 * index.vertex_index + 1];
            new_vert.position[2] = attrib.vertices[3 * index.vertex_index + 2];

            //vertex normal
            new_vert.normal[0] = attrib.normals[3 * index.normal_index + 0];
            new_vert.normal[1] = attrib.normals[3 * index.normal_index + 1];
            new_vert.normal[2] = attrib.normals[3 * index.normal_index + 2];

            //vertex uv
            if(index.texcoord_index >= 0){
                new_vert.texCoord.x = attrib.texcoords[2 * index.texcoord_index + 0];
                new_vert.texCoord.y = 1.0f - attrib.texcoords[2 * index.texcoord_index + 1];
            }

            //we are setting the vertex color as the vertex normal. This is just for display purposes
            new_vert.color = new_vert.normal;

            if (uniqueVertices.count(new_vert) == 0) {
                uniqueVertices[new_vert] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(new_vert);
            }
		
			indices.push_back(uniqueVertices[new_vert]);
        }
    }
    return true;
}
