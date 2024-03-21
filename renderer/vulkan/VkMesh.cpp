#include "VkMesh.hpp"
#include "VkStructures.hpp"
#include <tiny_obj_loader.h>
#include <unordered_map>

using namespace renderer;

vk::VertexInputBindingDescription Vertex::GetBindingDescription() {
    static vk::VertexInputBindingDescription binding;
    binding.setBinding(0)
        .setInputRate(vk::VertexInputRate::eVertex)
        .setStride(sizeof(Vertex));
    return binding;
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

    try {
        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename)) {
            CoreLog("Load %s failed, Error file path!", filename);
            return false;
        }
    } catch (const std::exception& e) {
        CoreLog("Load %s failed, Error file path!   %s", filename, e.what());
        return false;
    }

#ifdef _DEBUG_
	INFO("\n Loading %s :	\n\
	\t Shape count : %d		\n	\t Vertex count: %d		\n\
	\t Normal count: %d		\n	\t UV count: %d			\n\
	\t Sub Model count: %d	\n	\t Material count: %d",
	filename, shapes.size(), attrib.vertices.size() / 3, attrib.normals.size() / 3, attrib.texcoords.size() / 2, shapes.size(), materials.size());
#endif

	vertices.clear();
    indices.clear();

    // Loop over shapes
    for (const auto& shape : shapes) {
		// Loop over faces(polygon)
        for (const auto& index : shape.mesh.indices) {

            Vertex new_vert = {};

            //vertex position
            if (index.vertex_index >= 0) {
                new_vert.position[0] = attrib.vertices[index.vertex_index * 3 + 0];
                new_vert.position[1] = attrib.vertices[index.vertex_index * 3 + 1];
                new_vert.position[2] = attrib.vertices[index.vertex_index * 3 + 2];
            }

            //vertex normal
            if (index.normal_index >= 0) {
                new_vert.normal[0] = attrib.normals[index.normal_index * 3 + 0];
                new_vert.normal[1] = attrib.normals[index.normal_index * 3 + 1];
                new_vert.normal[2] = attrib.normals[index.normal_index * 3 + 2];
                new_vert.color = {0.5f, 0.5f, 0.5f};
            }

            //vertex uv
            if(index.texcoord_index >= 0){
                new_vert.texCoord.x = attrib.texcoords[index.texcoord_index * 2 + 0];
                new_vert.texCoord.y = 1.0f - attrib.texcoords[index.texcoord_index * 2 + 1];
            }

            //we are setting the vertex color as the vertex normal. This is just for display purposes
            if (uniqueVertices.count(new_vert) == 0) {
                uniqueVertices[new_vert] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(new_vert);
            }
		
			indices.push_back(uniqueVertices[new_vert]);
        }
    }

    CoreLog("Loaded %s", filename);
    return true;
}

void Particals::SetPartialCount(int size) {
    particals.resize(size);
    writeData.resize(size);

    for (int i = 0; i < size; ++i) {
        ParticalData partical;
        partical.position = { i, i + 1, i + 2, i + 3 };
        partical.color = { 1, 1, 1, 1 };
        partical.velocity = { 1, 0, 0, 0 };
        particals[i] = partical;
    }
}
