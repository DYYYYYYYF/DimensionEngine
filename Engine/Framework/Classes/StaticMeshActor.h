#pragma once

#include "Actor.h"
#include "Rendering/Resources/Geometry/Geometry.hpp"

#include <vector>

class AStaticMeshActor : public AActor{
public:
	DECLARE_CLASS_TYPE(AStaticMeshActor)

public:
	AStaticMeshActor() : AActor(), geometries(nullptr), geometry_count(0), Generation(INVALID_ID_U8){}
	AStaticMeshActor(const FString& Name) : AActor(Name), geometries(nullptr), geometry_count(0), Generation(INVALID_ID_U8){}
	virtual ~AStaticMeshActor() { Unload(); }

public:
	DAPI virtual void Draw();

	DAPI bool LoadFromResource(const FString& resource_name);
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

struct FMeshLoadParams {
	FString resource_name;
	AStaticMeshActor* out_mesh = nullptr;
	class UAsset mesh_resource;
};
