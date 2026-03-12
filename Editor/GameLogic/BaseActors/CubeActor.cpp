#include "CubeActor.h"
#include <Systems/GeometrySystem.h>

ACubeActor::ACubeActor() : ACubeActor("CubeActor") {}

ACubeActor::ACubeActor(const FString& Name) : AStaticMeshActor(Name) {
	// Default load cube model.
	geometry_count = 1;
	geometries = (Geometry**)Memory::Allocate(sizeof(Geometry*) * geometry_count, MemoryType::eMemory_Type_Array);
	SGeometryConfig GeoConfig = GeometrySystem::GenerateCubeConfig(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, "TestCube", "Material.Builtin.GBuffer");
	geometries[0] = GeometrySystem::AcquireFromConfig(GeoConfig, true);
	Generation = 0;

	// Clean up the allocations for the geometry config.
	GeometrySystem::ConfigDispose(&GeoConfig);
}

ACubeActor::~ACubeActor() {

}