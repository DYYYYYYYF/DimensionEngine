#pragma once

#include "Math/MathTypes.hpp"
#include "Framework/Components/TransformComponent.hpp"

class Material;

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
	void SetMaterialName(const std::string& mn) { material_name = mn; }
	const std::string& GetMaterialName() const { return material_name; }
	void SetName(const std::string& n) { name = n; }
	const std::string& GetName() const { return name; }

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

	std::string name;
	std::string material_name;
};

class Geometry {
public:
	DAPI void SetName(const std::string& n) { name = n; }
	DAPI std::string GetName() { return name; }

public:
	uint32_t ID;
	uint32_t InternalID;
	uint32_t Generation;
	Vector3 Center;
	Extents3D Extents;
	std::string name;
	Material* Material = nullptr;

};