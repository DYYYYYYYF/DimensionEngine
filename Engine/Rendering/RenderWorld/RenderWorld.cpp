#include "RenderWorld.h"
#include "Framework/Classes/CameraActor.h"
#include "Systems/CameraSystem.h"
#include "Systems/RenderViewSystem.hpp"
#include "Rendering/Resources/Geometry/Geometry.hpp"

void URenderWorld::Record(float DeltaTime) {
	// 这里临时调用Renderview
	FrustumCull();

	IRenderView* RenderViewDeferred = RenderViewSystem::Get().Get(ERenderViewType::Deferred);
	if (!RenderViewDeferred) return;
	IRenderView* RenderViewUI = RenderViewSystem::Get().Get(ERenderViewType::UI);
	if (!RenderViewUI) return;

	// 临时使用数组存放，后续可以作为成员变量
	TArray<FRenderProxy*> VisibleProxies;
	TArray<FRenderProxy*> UIProxies;
	for (FRenderProxy* Proxy : Proxies) {
		if (Proxy->GetRenderFeatureFlags() & ERenderFeature::DeferredLighting) {
			VisibleProxies.Push(Proxy);
		}
		if (Proxy->GetRenderFeatureFlags() & ERenderFeature::UI) {
			UIProxies.Push(Proxy);
		}
	}

	RenderViewDeferred->Render(VisibleProxies);
	RenderViewUI->Render(UIProxies);
}

void URenderWorld::AddProxy(FRenderProxy* Proxy) {
	Proxies.Push(Proxy);
}

void URenderWorld::RemoveProxy(FRenderProxy* Proxy) {
	for (uint32_t i = 0; i < (uint32_t)Proxies.Size(); ++i) {
		if (Proxies[i] == Proxy) {
			Proxies.PopAt(i);
			break;
		}
	}	
}

uint64_t URenderWorld::GetVisibleGeometryCount() const {
	return VisibleCount;
}

void URenderWorld::FrustumCull() {
	// 重置可见对象数量（geometry为最小单位）
	VisibleCount = 0;
	for (uint32_t i = 0; i < (uint32_t)Proxies.Size(); ++i) {

		// 只有特定类型需要参与剔除（暂时只有StaticMesh）
		FStaticMeshRenderProxy* Proxy = Cast<FStaticMeshRenderProxy*>(Proxies[i]);
		if (!Proxy || (Proxy->GetRenderFeatureFlags() & ERenderFeature::DeferredLighting) == 0) continue;

		// 获取视锥体
		ACameraActor* WorldCamera = CameraSystem::Get().GetMainCamera();
		if (!WorldCamera) return;
		FFrustum CameraFrustum = WorldCamera->GetFrustum();

		// 模型初筛
		Matrix4 ModelMat = Proxy->GetModelMatrix();
		const Extents3D& Extents = Proxy->GetBoundingBox();
		Extents3D PostExtents = TransformBounds(Extents, ModelMat);

		bool IsMeshVisible = FrustumCullInside(CameraFrustum, PostExtents);
		Proxy->SetVisibility(IsMeshVisible);

		// 如果模型可见再判断是否模型内所有Submesh都可见
		if (IsMeshVisible) {
			TArray<UGeometry*> SubMeshes = Proxy->GetMesh();
			for (UGeometry* Geometry : SubMeshes) {
				const Extents3D& GeometryExtents = Geometry->GetBoundingBox();
				Extents3D PostGeometryExtents = TransformBounds(GeometryExtents, ModelMat);
				if (FrustumCullInside(CameraFrustum, PostGeometryExtents)) {
					Geometry->SetVisibility(true);
					VisibleCount++;
				}
			}
		}
	}
}

Extents3D URenderWorld::TransformBounds(const Extents3D& LocalBounds, const Matrix4& ModelMatrix) {
	Vector3 ExtensMin = LocalBounds.min.Transform(ModelMatrix);
	Vector3 ExtensMax = LocalBounds.max.Transform(ModelMatrix);
	return Extents3D{ ExtensMin, ExtensMax };
}

bool URenderWorld::FrustumCullInside(const FFrustum& Frustum, const Extents3D& Extent) {
	Vector3 ExtentsMin = Extent.min;
	Vector3 ExtentsMax = Extent.max;
	Vector3 Center = (ExtentsMin + ExtentsMax) * 0.5f;
	switch (CullMode)
	{
		// Bounding sphere calculation
	case FrustumCullMode::eSphere_Cull:
	{
		Vector3 HalfExtents = (ExtentsMax - ExtentsMin) * 0.5f;
		float Radius = HalfExtents.Length();
		return Frustum.IntersectsSphere(Center, Radius);
	} break;
	// AABB calculation
	case FrustumCullMode::eAABB_Cull:
	{
		Vector3 HalfExtents = {
					Dabs(ExtentsMax.x - Center.x),
					Dabs(ExtentsMax.y - Center.y),
					Dabs(ExtentsMax.z - Center.z)
		};

		return Frustum.IntersectsAABB(Center, HalfExtents);
	} break;
	}

	return false;
}
