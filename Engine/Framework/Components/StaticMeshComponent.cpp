#include "StaticMeshComponent.h"
#include "Rendering/RenderWorld/RenderProxy.h"
#include "Rendering/Resources/Geometry/Geometry.hpp"
#include "Framework/Classes/Actor.h"

UStaticMeshComponent::UStaticMeshComponent(const FString& Name) : UMeshComponent(Name){
	Name_ = Name;
}

void UStaticMeshComponent::DrawMesh() {

}

bool UStaticMeshComponent::CreateRenderProxy() {
	RenderProxy = NewObject<FStaticMeshRenderProxy>(MemoryType::eMemory_Type_Renderer);
	if (!RenderProxy) {
		GLOG(Log::eError, "Failed to create RenderProxy for StaticMeshComponent.");
		return false;
	}

	return true;
}

void UStaticMeshComponent::UpdateRenderProxy() {
	// 还未注册到场景
	if (!IsRegistered) return;

	FStaticMeshRenderProxy* Proxy = Cast<FStaticMeshRenderProxy*>(RenderProxy);
	if (!Proxy) {
		GLOG(Log::eError, "UStaticMeshComponent RenderProxy is null. Cannot update.");
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner) {
		GLOG(Log::eError, "StaticMeshComponent has no owner.");
		return;
	}

	if (Mesh.IsEmpty()) {
		GLOG(Log::eWarn, "StaticMeshComponent has no mesh assigned.");
		return;
	}

	// 填充数据
	Proxy->SetMesh(Mesh);
	Proxy->SetBoundingBox(GetBoundingBox());
	Proxy->SetModelMatrix(Owner->GetWorldTransform());
	Proxy->SetUniqueID(Owner->GetUniqueID());
}

void UStaticMeshComponent::SetMesh(TArray<UGeometry*> InMesh) {
	// 释放被替换掉的旧几何引用：旧几何若不在新数组中，则引用计数减一。
	// 此前直接覆盖 Mesh 数组导致旧几何的引用计数永不归零，
	// Geometry/MaterialInstance/GPU buffer 每轮切换场景持续泄漏
	// （压测中 Array 池 414MiB、父材质引用计数 516 居高不下）。
	for (UGeometry* G : Mesh) {
		if (!G) continue;

		bool bStillReferenced = false;
		for (UGeometry* NG : InMesh) {
			if (NG == G) {
				bStillReferenced = true;
				break;
			}
		}
		if (!bStillReferenced) {
			G->DecreaseReferenceCount();
		}
	}

	Mesh = InMesh;
	UpdateBounding();
}

void UStaticMeshComponent::ClearGeometries() {
	// 仅清空数组，不递减引用：调用方（Actor::Unload）已负责 Decrease，
	// 避免对已销毁几何二次 Decrease 造成 Use-After-Free。
	Mesh.Clear();
}

void UStaticMeshComponent::UpdateBounding() {
	Extents3D Bounding;

	for (UGeometry* Geometry : Mesh) {
		const Extents3D GeometryBounding = Geometry->GetBoundingBox();
		// X
		Bounding.min.x = DMIN(Bounding.min.x, GeometryBounding.min.x);
		Bounding.max.x = DMAX(Bounding.max.x, GeometryBounding.max.x);
		// Y
		Bounding.min.y = DMIN(Bounding.min.y, GeometryBounding.min.y);
		Bounding.max.y = DMAX(Bounding.max.y, GeometryBounding.max.y);
		// Z
		Bounding.min.z = DMIN(Bounding.min.z, GeometryBounding.min.z);
		Bounding.max.z = DMAX(Bounding.max.z, GeometryBounding.max.z);
	}

	SetBoundingBox(Bounding);
}
