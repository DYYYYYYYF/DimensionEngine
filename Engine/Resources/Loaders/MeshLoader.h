#pragma once

#include "Resources/Loaders/IResourceLoader.hpp"
#include "Resources/Mesh.hpp"

struct FileHandle;
struct SGeometryConfig;
struct SMaterialConfig;

class MeshLoader : public IResourceLoader {
public:
	MeshLoader();

public:
	virtual bool Load(const char* name, void* params, Resource* resource) override;
	virtual void Unload(Resource* resource) override;

private:
	virtual bool ImportObjFile(FileHandle* obj_file, const char* out_dsm_filename, std::vector<SGeometryConfig>& out_geometries);
	virtual void ProcessSubobject(std::vector<Vec3>& positions, std::vector<Vec3>& normals, std::vector<Vec2>& texcoords, std::vector<MeshFaceData>& faces, SGeometryConfig* out_data);
	virtual bool ImportObjMaterialLibraryFile(const char* mtl_file_path);

	virtual bool LoadDsmFile(FileHandle* dsm_file, std::vector<SGeometryConfig>& out_geometries);
	virtual bool WriteDsmFile(const char* path, const char* name, uint32_t geometry_count, std::vector<SGeometryConfig>& geometries);
	virtual bool WriteDmtFile(const char* mtl_file_path, SMaterialConfig* config);

};
