#pragma once

#include "Actor.h"
#include "Rendering/Resources/Geometry/Geometry.hpp"

#include <vector>

class UGeometry;
class UStaticMeshComponent;

class ENGINE_API AStaticMeshActor : public AActor{
public:
	DECLARE_CLASS_TYPE(AStaticMeshActor)

public:
	AStaticMeshActor(const FString& Name);
	virtual ~AStaticMeshActor() { Unload(); }

public:
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
	UGeometry* GeometryAsset;

	UStaticMeshComponent* MeshComponent = nullptr;
};
