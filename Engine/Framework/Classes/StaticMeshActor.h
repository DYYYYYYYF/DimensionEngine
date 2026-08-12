#pragma once

#include "Actor.h"
#include "Rendering/Resources/Geometry/Geometry.hpp"

#include <vector>

class UGeometry;
class UStaticMeshComponent;

struct FMeshLoadParams {
	FString resource_name;
	class AStaticMeshActor* out_mesh = nullptr;
	UAsset mesh_resource;
};

class ENGINE_API AStaticMeshActor : public AActor{
public:
	DECLARE_CLASS_TYPE(AStaticMeshActor)

public:
	AStaticMeshActor(const FString& Name);
	virtual ~AStaticMeshActor() { Unload(); }

public:
	virtual void Draw();

	bool LoadFromResource(const FString& resource_name);
	void Unload();

	bool SetMeshResource(UGeometry* mesh_resource);

private:
	void LoadJobSuccess();
	void LoadJobFail();
	bool LoadJobStart();

public:
	unsigned char Generation;
	unsigned short geometry_count;
	UGeometry** geometries;

	UStaticMeshComponent* MeshComponent = nullptr;

	struct FMeshLoadParams LoadParams;
};
