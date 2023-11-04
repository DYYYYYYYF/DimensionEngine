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

    if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename)){
        std::runtime_error("Error: " + err);
    }

    // std::unordered_map<Vertex, uint32_t> uniqueVertices;
    vertices.clear();
    
    //attrib will contain the vertex arrays of the file
    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++) {
		    // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            //hardcode loading to triangles
            int fv = 3;

            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++) {
                //copy it into our vertex
                Vertex new_vert;

                // access to vertex
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                //vertex position
                tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
                tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
                tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
                //vertex normal
                tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
                tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
                tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];

                //vertex uv
                if(idx.texcoord_index >= 0){
                    tinyobj::real_t ux = attrib.texcoords[2 * idx.texcoord_index + 0];
                    tinyobj::real_t uy = attrib.texcoords[2 * idx.texcoord_index + 1];
                    new_vert.texCoord.x = ux;
                    new_vert.texCoord.y = 1-uy;
                }

                new_vert.position[0] = vx;
                new_vert.position[1] = vy;
                new_vert.position[2] = vz;

                new_vert.normal[0] = nx;
                new_vert.normal[1] = ny;
                new_vert.normal[2] = nz;

                //we are setting the vertex color as the vertex normal. This is just for display purposes
                new_vert.color = new_vert.normal;

                // if(uniqueVertices.count(new_vert) == 0){
                //     uniqueVertices[new_vert] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(new_vert);
                //     std::cout << vertices.size() << std::endl;
                // }
            }
        index_offset += fv;
        }
    }
    return true;
}
