#pragma once

#include "Math/MathTypes.hpp"
#include "Containers/FString.hpp"

enum MeshFileType {
	eMesh_File_Type_Not_Found,
	eMesh_File_Type_DSM,
	eMesh_File_Type_3D_Model
};

struct SupportedMeshFileType {
	std::string extension;
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

struct SGeometryConfig {
public:
	void SetMaterialName(const FString& mn) { material_name = mn; }
	const FString& GetMaterialName() const { return material_name; }
	void SetName(const FString& n) { name = n; }
	const FString& GetName() const { return name; }

	// Vertices
	uint32_t vertex_size = 0;
	uint32_t vertex_count = 0;
	void* vertices = nullptr;

	// Indices
	uint32_t index_size = 0;
	uint32_t index_count = 0;
	void* indices = nullptr;

	Vector3 center;
	Vector3 min_extents;
	Vector3 max_extents;

	FString name;
	FString material_name;
};
