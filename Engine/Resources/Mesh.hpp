#pragma once

#include "Resource.hpp"
#include "Geometry.hpp"
#include "Math/Transform.hpp"
#include <vector>

enum MeshFileType {
	eMesh_File_Type_Not_Found,
	eMesh_File_Type_DSM,
	eMesh_File_Type_OBJ
};

struct SupportedMeshFileType {
	const char* extension = nullptr;
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

class DAPI Mesh {
public:
	Mesh() : geometries(nullptr), geometry_count(0) {}
	bool LoadFromResource(const char* resource_name);
	void Unload();
	void ReloadMaterial(const char* mat_name = nullptr);

private:
	static void LoadJobSuccess(void* params);
	static void LoadJobFail(void* params);
	static bool LoadJobStart(void* params, void* result_data);


public:
	uint32_t UniqueID;
	unsigned char Generation;
	unsigned short geometry_count;
	Geometry** geometries = nullptr;
	Transform Transform;
};

struct MeshLoadParams {
	const char* resource_name = nullptr;
	Mesh* out_mesh = nullptr;
	class Resource mesh_resource;
};
