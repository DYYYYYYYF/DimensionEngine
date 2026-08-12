#pragma once

#include "RenderProxy.h"
#include "Math/Frustum.hpp"
#include "Containers/TArray.hpp"

class URenderWorld {
public:
	void Record(float DeltaTime);

public:
	void AddProxy(FRenderProxy* Proxy);
	void RemoveProxy(FRenderProxy* Proxy);

	uint64_t GetVisibleGeometryCount() const;

private:
	void FrustumCull();
	Extents3D TransformBounds(const Extents3D& LocalBounds, const Matrix4& ModelMatrix);
	bool FrustumCullInside(const FFrustum& Frustum, const Extents3D& Extent);

private:
	uint64_t VisibleCount = 0;
	TArray<FRenderProxy*> Proxies;

	FrustumCullMode CullMode = FrustumCullMode::eAABB_Cull;

};