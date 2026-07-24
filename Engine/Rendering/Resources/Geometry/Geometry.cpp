#include "Geometry.hpp"
#include "Systems/GeometrySystem.h"
#include "Rendering/Resources/Material/Material.hpp"

Geometry::Geometry(){
	InternalID = INVALID_ID;
	AssetType = EAssetType::Geometry;
}

Geometry::Geometry(const FString& name) : UAsset(name){
	InternalID = INVALID_ID;
	AssetType = EAssetType::Geometry;
}

Geometry::~Geometry() {
	if (ReferenceCount > 0 && AutoRelease) {
		GLOG(Log::eWarn, "Geometry '%s' is being destroyed while still in use. Reference count: %zu", name.CStr(), ReferenceCount);
	}
}

void Geometry::DecreaseReferenceCount(uint32_t count/* = 1*/) {
	if (ReferenceCount <= 0) return;

	// Also decrease the reference count of the associated material.
	if (Material) {
		Material->DecreaseReferenceCount();
	}

	ReferenceCount -= count;
	if (ReferenceCount == 0) {
		// Geometry is no longer in use, trigger cleanup.
		DestroyInstance();
	}
}


void Geometry::DestroyInstance() {
	GeometrySystem::Get().DestroyGeometry(this);
}
