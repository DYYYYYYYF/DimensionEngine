#pragma once

#include "Math/MathTypes.hpp"
#include "Containers/FString.hpp"

enum MeshFileType {
	eMesh_File_Type_Not_Found,
	eMesh_File_Type_DSM,
	eMesh_File_Type_3D_Model
};

struct FSupportedMeshFileType {
	std::string extension;
	MeshFileType type;
	bool is_binary;
};

struct FMeshVertexIndexData {
	uint32_t position_index;
	uint32_t normal_index;
	uint32_t texcoord_index;
};

struct FMeshFaceData {
	FMeshVertexIndexData vertices[3];
};

struct FGeometryConfig {
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

	// 为 true 时使用 host-visible 顶点/索引缓冲（map/copy 直写，无 staging/waitIdle）。
	// 供 UI 合批等每帧高频小量上传使用，需配合后端双缓冲轮转。
	bool bHostVisible = false;

	Vector3 center;
	Vector3 min_extents;
	Vector3 max_extents;

	FString name;
	FString material_name;
};
