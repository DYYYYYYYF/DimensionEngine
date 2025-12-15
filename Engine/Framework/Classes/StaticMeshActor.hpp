#pragma once

#include "MeshActor.h"
#include "Resources/Resource.hpp"
#include "Resources/Geometry.hpp"
#include "Math/Transform.hpp"
#include <vector>

class StaticMeshActor : public MeshActor{
public:
	StaticMeshActor() : MeshActor(), geometries(nullptr), geometry_count(0), Generation(INVALID_ID_U8){}
	StaticMeshActor(std::string Name) : MeshActor(Name), geometries(nullptr), geometry_count(0), Generation(INVALID_ID_U8){}
	virtual ~StaticMeshActor() { Unload(); }

public:
	DAPI virtual void Draw() override;

	DAPI bool LoadFromResource(const std::string& resource_name);
	DAPI void Unload();

private:
	void LoadJobSuccess(void* params);
	void LoadJobFail(void* params);
	bool LoadJobStart(void* params, void* result_data);

public:
	unsigned char Generation;
	unsigned short geometry_count;
	Geometry** geometries;
};

struct MeshLoadParams {
	std::string resource_name;
	StaticMeshActor* out_mesh = nullptr;
	class Resource mesh_resource;
};
