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
	RenderProxy = NewObject<FRenderProxy>(MemoryType::eMemory_Type_Renderer);
	if (!RenderProxy) {
		GLOG(Log::eError, "Failed to create RenderProxy for StaticMeshComponent.");
		return false;
	}

	UpdateRenderProxy();
	return true;
}

void UStaticMeshComponent::UpdateRenderProxy() {
	if (!RenderProxy) {
		GLOG(Log::eError, "RenderProxy is null. Cannot update.");
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
	RenderProxy->SetMesh(Mesh);
	RenderProxy->SetRenderFeatureFlags(ERenderFeature::DeferredLighting);
	RenderProxy->SetBoundingBox(GetBoundingBox());
	RenderProxy->SetModelMatrix(Owner->GetWorldTransform());
	RenderProxy->SetUniqueID(Owner->GetUniqueID());
}

void UStaticMeshComponent::SetMesh(TArray<UGeometry*> InMesh) {
	Mesh = InMesh;
	UpdateBounding();
}

void UStaticMeshComponent::UpdateBounding() {
	Extents3D Bounding;

	for (UGeometry* Geometry : Mesh) {
		const Extents3D GeometryBounding = Geometry->GetBoundingBox();
		// X
		Bounding.min.x = DMIN(Bounding.min.x, GeometryBounding.min.x);
		Bounding.max.x = DMIN(Bounding.max.x, GeometryBounding.max.x);
		// Y
		Bounding.min.y = DMIN(Bounding.min.y, GeometryBounding.min.y);
		Bounding.max.y = DMIN(Bounding.max.y, GeometryBounding.max.y);
		// Z
		Bounding.min.z = DMIN(Bounding.min.z, GeometryBounding.min.z);
		Bounding.max.z = DMIN(Bounding.max.z, GeometryBounding.max.z);
	}

	SetBoundingBox(Bounding);
}
