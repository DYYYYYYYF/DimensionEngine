#pragma once

#include "Math/MathTypes.hpp"
#include "Containers/TArray.hpp"

class UGeometry;

enum ERenderFeature : uint32_t {
	None = 0,
	ForwardLighting = 1 << 1,
	DeferredLighting = 1 << 2,
	UI = 1 << 3,
	Lighting = 1 << 4,
};

class FRenderProxy {
public:
	FRenderProxy() : bVisible(true), DistanceToCamera(0.0f) {}

	void SetModelMatrix(const Matrix4& InModelMatrix) { ModelMatrix = InModelMatrix; }
	const Matrix4& GetModelMatrix() const { return ModelMatrix; }

	void SetVisibility(bool bInVisible) { bVisible = bInVisible; }
	bool IsVisible() const { return bVisible; }

	void SetBoundingBox(const Extents3D& InBoundingBox) { BoundingBox = InBoundingBox; }
	const Extents3D& GetBoundingBox() const { return BoundingBox; }

	void SetDistanceToCamera(float InDistance) { DistanceToCamera = InDistance; }
	float GetDistanceToCamera() const { return DistanceToCamera; }

	void SetMesh(TArray<UGeometry*> InMesh) { Mesh = InMesh; }
	TArray<UGeometry*> GetMesh() const { return Mesh; }

	void SetUniqueID(uint64_t InUniqueID) { UniqueID = InUniqueID; }
	uint64_t GetUniqueID() const { return UniqueID; }

	void SetRenderFeatureFlags(uint32_t InFlags) { RenderFeatureFlags |= InFlags; }
	uint32_t GetRenderFeatureFlags() const { return RenderFeatureFlags; }

protected:
	uint32_t RenderFeatureFlags = ERenderFeature::None; // Bitmask for render features
	Matrix4 ModelMatrix;   // Transformation matrix for the proxy
	bool bVisible;        // Visibility flag for rendering
	Extents3D BoundingBox; // Axis-aligned bounding box for culling
	float DistanceToCamera; // Distance from the camera for sorting purposes
	TArray<UGeometry*> Mesh;
	uint64_t UniqueID = INVALID_ID;
};