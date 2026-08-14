#include "Geometry.hpp"
#include "Systems/GeometrySystem.h"
#include "Rendering/Resources/Material/Material.hpp"

UGeometry::UGeometry(const FString& name) : UAsset(name){
	InternalID = INVALID_ID;
	AssetType = EAssetType::Geometry;
}

UGeometry::~UGeometry() {
	DecreaseReferenceCount();

	if (ReferenceCount > 0 && AutoRelease) {
		GLOG(Log::eWarn, "Geometry '%s' is being destroyed while still in use. Reference count: %zu", name.CStr(), ReferenceCount);
	}
}

void UGeometry::SetMaterial(UMaterial* Mat) {
	if (MaterialInstance) {
		MaterialInstance->Release();
	}

	if (Mat) {
		MaterialInstance = NewObject<UMaterialInstance>(Mat);
	}
	else {
		MaterialInstance = nullptr;
	}
}

void UGeometry::DecreaseReferenceCount(uint32_t count/* = 1*/) {
	if (ReferenceCount <= 0) return;

	if (MaterialInstance) {
		MaterialInstance->Release();
	}

	ReferenceCount -= count;
	if (ReferenceCount == 0) {
		// Geometry is no longer in use, trigger cleanup.
		DestroyInstance();
	}
}


void UGeometry::DestroyInstance() {
	// delete the associated material.
	if (MaterialInstance) {
		DeleteObject(MaterialInstance);
		MaterialInstance = nullptr;
	}

	GeometrySystem::Get().DestroyGeometry(this);
}
