#pragma once

#include "Rendering/Interface/IResourceLoader.hpp"
#include "Rendering/Resources/Geometry/Geometry.hpp"
#include <vector>
#include <unordered_map>

#define DEFAULT_SHADER "Shader.Builtin.GBuffer"

struct FileHandle;
struct SGeometryConfig;
struct SMaterialConfig;

// Assimp前向声明
struct aiScene;
struct aiNode;
struct aiMesh;
struct aiMaterial;

class MeshLoader : public IResourceLoader {
public:
	MeshLoader();

public:
	virtual bool Load(const FString& name, void* params, UAsset* resource) override;
	virtual void Unload(UAsset* resource) override;

private:
	// 通用文件处理
	virtual void ProcessSubobject(std::vector<Vector3>& positions, std::vector<Vector3>& normals, std::vector<Vector2f>& texcoords, std::vector<MeshFaceData>& faces, SGeometryConfig* out_data);
	virtual bool LoadDsmFile(const FString& path, std::vector<SGeometryConfig>& out_geometries);
	virtual bool WriteDsmFile(const FString& path, const FString& name, std::vector<SGeometryConfig>& geometries);
	virtual bool WriteDmtFile(const FString& mtl_file_path, SMaterialConfig* config);

	// 统一的3D模型文件处理 (使用Assimp) - 支持OBJ, GLTF, FBX, DAE等
	virtual bool Import3DModelFile(const FString& model_file, const FString& out_dsm_filename, std::vector<SGeometryConfig>& out_geometries);

	// Assimp相关处理函数
	virtual bool ProcessAssimpMaterials(const aiScene* scene, const FString& out_dsm_filename, std::vector<SMaterialConfig>& materialConfigs);
	virtual void ProcessAssimpTextures(const aiMaterial* mat, SMaterialConfig& config);
	virtual void ProcessAssimpNode(aiNode* node, const aiScene* scene, const std::vector<SMaterialConfig>& materialConfigs, const Matrix4& parentTransform, std::vector<SGeometryConfig>& out_geometries);
	virtual void ProcessAssimpMesh(aiMesh* mesh, const aiScene* scene, const std::vector<SMaterialConfig>& materialConfigs, const Matrix4& transform, std::vector<SGeometryConfig>& out_geometries);

	virtual bool ImportObjFile(const FString& obj_file, const FString& out_dsm_filename, std::vector<SGeometryConfig>& out_geometries) {
		return Import3DModelFile(obj_file, out_dsm_filename, out_geometries);
	}
	virtual bool ImportGltfFile(const FString& gltf_file, const FString& out_dsm_filename, std::vector<SGeometryConfig>& out_geometries) {
		return Import3DModelFile(gltf_file, out_dsm_filename, out_geometries);
	}

	virtual bool DeduplicateGeometry(std::vector<SGeometryConfig>& out_geometries);
};