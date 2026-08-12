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

private:
	void FrustumCull();
	bool FrustumCullInside(const FFrustum& Frustum, const Extents3D& Extent);

private:
	TArray<FRenderProxy*> Proxies;

	FrustumCullMode CullMode = FrustumCullMode::eAABB_Cull;

};