#include "CubeActor.h"
#include <Systems/GeometrySystem.h>
#include <Framework/Classes/StaticMeshActor.h>

ACubeActor::ACubeActor() : ACubeActor("CubeActor") {}

ACubeActor::ACubeActor(const FString& Name) : AStaticMeshActor(Name) {
	// Default load cube model.
	geometry_count = 1;
	GeometrySystem& GeoSys = GeometrySystem::Get();
	GeometryAsset = (UGeometry*)Memory::Allocate(sizeof(UGeometry) * geometry_count, MemoryType::eMemory_Type_Array);
	FGeometryConfig GeoConfig = GeoSys.GenerateCubeConfig(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, "TestCube", "Material.Builtin.GBuffer");
	GeometryAsset = GeoSys.AcquireFromConfig(GeoConfig, true);
	Generation = 0;
	GeometryAsset->IncreaseReferenceCount();

	// Clean up the allocations for the geometry config.
	GeoSys.ConfigDispose(&GeoConfig);

	SetMeshResource(GeometryAsset);
}

ACubeActor::~ACubeActor() {

}