#include "CubeActor.h"
#include <Systems/GeometrySystem.h>
#include <Framework/Classes/StaticMeshActor.h>

ACubeActor::ACubeActor() : ACubeActor("CubeActor") {}

ACubeActor::ACubeActor(const FString& Name) : AStaticMeshActor(Name) {
	// Default load cube model.
	geometry_count = 1;
	GeometrySystem& GeoSys = GeometrySystem::Get();
	// NOTE: AcquireFromConfig 内部已为返回的几何体增加一次引用计数（+1），
	// 这里不再预先裸 Allocate（会被返回值覆盖导致泄漏），也不再显式 IncreaseReferenceCount（避免双重 +1）。
	FGeometryConfig GeoConfig = GeoSys.GenerateCubeConfig(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, "TestCube", "Material.Builtin.GBuffer");
	GeometryAsset = GeoSys.AcquireFromConfig(GeoConfig, true);
	Generation = 0;

	// Clean up the allocations for the geometry config.
	GeoSys.ConfigDispose(&GeoConfig);

	SetMeshResource(GeometryAsset);
}

ACubeActor::~ACubeActor() {

}