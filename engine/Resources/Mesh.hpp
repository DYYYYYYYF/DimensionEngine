#pragma once

#include "Geometry.hpp"
#include "Math/Transform.hpp"
#include <vector>

enum MeshFileType {
	eMesh_File_Type_Not_Found,
	eMesh_File_Type_DSM,
	eMesh_File_Type_OBJ
};

struct SupportedMeshFileType {
	char* extension = nullptr;
	MeshFileType type;
	bool is_binary;
};

struct MeshVertexIndexData {
	uint32_t position_index;
	uint32_t normal_index;
	uint32_t texcoord_index;
};

struct MeshFaceData {
	MeshVertexIndexData vertices[3];
};

struct MeshGroupData {
	std::vector<MeshFaceData> Faces;
};

struct Mesh {
	unsigned short geometry_count;
	Geometry** geometries = nullptr;
	Transform Transform;
};