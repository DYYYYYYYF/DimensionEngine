#include "CubeActor.h"
#include <Systems/GeometrySystem.h>

ACubeActor::ACubeActor() : ACubeActor("CubeActor") {}

ACubeActor::ACubeActor(const FString& Name) : AStaticMeshActor(Name) {
	// Default load cube model.
	geometry_count = 1;
	GeometrySystem& GeoSys = GeometrySystem::Get();
	geometries = (UGeometry**)Memory::Allocate(sizeof(UGeometry*) * geometry_count, MemoryType::eMemory_Type_Array);
	SGeometryConfig GeoConfig = GeoSys.GenerateCubeConfig(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, "TestCube", "Material.Builtin.GBuffer");
	geometries[0] = GeoSys.AcquireFromConfig(GeoConfig, true);
	Generation = 0;
	geometries[0]->IncreaseReferenceCount();

	// Clean up the allocations for the geometry config.
	GeoSys.ConfigDispose(&GeoConfig);
}

ACubeActor::~ACubeActor() {

}