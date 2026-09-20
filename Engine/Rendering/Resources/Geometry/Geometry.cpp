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

	// NOTE: 此处不再对 MaterialInstance 提前 Release()。
	// UMaterialInstance 对父材质的引用计数只由“构造 +1 / 析构 -1”维护，
	// 若在引用归零前提前 Release 一次，DestroyInstance 中 DeleteObject 触发析构会再次 -1，
	// 造成双减：共享同一材质的多个几何体会在某个实例释放后把父材质引用减到 0 并销毁，
	// 其余实例析构时对已销毁材质访问导致 Use-After-Free。
	// 引用归零统一在 DestroyInstance() 内通过 DeleteObject(MaterialInstance) 的析构递减。

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
